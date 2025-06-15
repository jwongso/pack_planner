using Microsoft.OpenApi.Models;
using System.Reflection;
using PackPlanner.Api.Middleware;
using PackPlanner.Api.Configuration;

var builder = WebApplication.CreateBuilder(args);

// Add services to the container
builder.Services.AddControllers();

// Configure API Explorer for OpenAPI/Swagger
builder.Services.AddEndpointsApiExplorer();

// Configure rate limiting options from appsettings.json
builder.Services.Configure<RateLimitingOptions>(
    builder.Configuration.GetSection(RateLimitingOptions.SectionName));

// Register rate limiting options as a singleton for middleware access
builder.Services.AddSingleton(provider =>
{
    var configuration = provider.GetRequiredService<IConfiguration>();
    var options = new RateLimitingOptions();
    configuration.GetSection(RateLimitingOptions.SectionName).Bind(options);
    return options;
});

// Configure Swagger/OpenAPI with comprehensive documentation
builder.Services.AddSwaggerGen(c =>
{
    c.SwaggerDoc("v1", new OpenApiInfo
    {
        Title = "PackPlanner API",
        Version = "v1",
        Description = "RESTful API for optimizing item packing into containers with various strategies and constraints. " +
                     "Includes built-in rate limiting to prevent abuse and ensure fair usage.",
        Contact = new OpenApiContact
        {
            Name = "PackPlanner API Support",
            Email = "support@packplanner.com"
        }
    });

    // Include XML comments for better API documentation
    var xmlFile = $"{Assembly.GetExecutingAssembly().GetName().Name}.xml";
    var xmlPath = Path.Combine(AppContext.BaseDirectory, xmlFile);
    if (File.Exists(xmlPath))
    {
        c.IncludeXmlComments(xmlPath);
    }

    // Add response examples and better error documentation
    c.EnableAnnotations();

    // Document rate limiting in Swagger
    c.AddSecurityDefinition("RateLimit", new OpenApiSecurityScheme
    {
        Type = SecuritySchemeType.ApiKey,
        In = ParameterLocation.Header,
        Name = "X-RateLimit-Info",
        Description = "API includes rate limiting. Check response headers for limit information."
    });
});

// Add CORS support for cross-origin requests
builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowAll", policy =>
    {
        policy.AllowAnyOrigin()
              .AllowAnyMethod()
              .AllowAnyHeader()
              .WithExposedHeaders("X-RateLimit-Limit", "X-RateLimit-Remaining",
                                "X-RateLimit-Reset", "X-RateLimit-Window", "Retry-After");
    });
});

// Add health checks
builder.Services.AddHealthChecks();

// Configure logging
builder.Logging.ClearProviders();
builder.Logging.AddConsole();
builder.Logging.AddDebug();

// Configure JSON serialization options
builder.Services.ConfigureHttpJsonOptions(options =>
{
    options.SerializerOptions.PropertyNamingPolicy = System.Text.Json.JsonNamingPolicy.CamelCase;
    options.SerializerOptions.WriteIndented = true;
});

var app = builder.Build();

// Configure the HTTP request pipeline

// Enable Swagger in all environments for this demo API
app.UseSwagger();
app.UseSwaggerUI(c =>
{
    c.SwaggerEndpoint("/swagger/v1/swagger.json", "PackPlanner API v1");
    c.RoutePrefix = string.Empty; // Serve Swagger UI at root
    c.DocumentTitle = "PackPlanner API Documentation";
    c.DefaultModelsExpandDepth(-1); // Hide models section by default
});

// Enable CORS
app.UseCors("AllowAll");

// Add rate limiting middleware - IMPORTANT: Must be early in pipeline
app.UseMiddleware<RateLimitingMiddleware>();

// Add security headers middleware
app.Use(async (context, next) =>
{
    context.Response.Headers["X-Content-Type-Options"] = "nosniff";
    context.Response.Headers["X-Frame-Options"] = "DENY";
    context.Response.Headers["X-XSS-Protection"] = "1; mode=block";
    await next();
});

// Enable HTTPS redirection in production
if (!app.Environment.IsDevelopment())
{
    app.UseHttpsRedirection();
}

// Map health check endpoint
app.MapHealthChecks("/health");

// Map controllers
app.MapControllers();

// Add a simple root endpoint that redirects to Swagger
app.MapGet("/", () => Results.Redirect("/swagger"));

// Global exception handling middleware
app.UseExceptionHandler(errorApp =>
{
    errorApp.Run(async context =>
    {
        context.Response.StatusCode = 500;
        context.Response.ContentType = "application/json";

        var error = new
        {
            error = "Internal Server Error",
            details = "An unexpected error occurred while processing the request",
            timestamp = DateTime.UtcNow
        };

        await context.Response.WriteAsync(System.Text.Json.JsonSerializer.Serialize(error));
    });
});

// Log startup information including rate limiting status
var logger = app.Services.GetRequiredService<ILogger<Program>>();
var rateLimitOptions = app.Services.GetRequiredService<RateLimitingOptions>();

logger.LogInformation("PackPlanner API starting up...");
logger.LogInformation("Environment: {Environment}", app.Environment.EnvironmentName);
logger.LogInformation("Rate Limiting: {Status}",
    rateLimitOptions.GlobalSettings.EnableRateLimiting ? "Enabled" : "Disabled");

if (rateLimitOptions.GlobalSettings.EnableRateLimiting)
{
    logger.LogInformation("Rate Limit Policies:");
    logger.LogInformation("  - Pack Planning: {Limit} requests per {Window}m",
        rateLimitOptions.EndpointPolicies.PackPlanning.RequestLimit,
        rateLimitOptions.EndpointPolicies.PackPlanning.WindowSizeMinutes);
    logger.LogInformation("  - Async Pack Planning: {Limit} requests per {Window}m",
        rateLimitOptions.EndpointPolicies.AsyncPackPlanning.RequestLimit,
        rateLimitOptions.EndpointPolicies.AsyncPackPlanning.WindowSizeMinutes);
    logger.LogInformation("  - Health Check: {Limit} requests per {Window}m",
        rateLimitOptions.EndpointPolicies.HealthCheck.RequestLimit,
        rateLimitOptions.EndpointPolicies.HealthCheck.WindowSizeMinutes);
}

logger.LogInformation("Swagger UI available at: {SwaggerUrl}",
    app.Environment.IsDevelopment() ? "http://localhost:5000" : "Application root");

app.Run();
