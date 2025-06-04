using System.Diagnostics;

namespace PackPlanner;

/// <summary>
/// Utility class for measuring execution time with high precision
/// </summary>
public class Timer
{
    private readonly Stopwatch _stopwatch;
    private bool _isRunning;

    /// <summary>
    /// Initializes a new instance of the Timer class
    /// </summary>
    public Timer()
    {
        _stopwatch = new Stopwatch();
        _isRunning = false;
    }

    /// <summary>
    /// Start timing
    /// </summary>
    public void Start()
    {
        _stopwatch.Restart();
        _isRunning = true;
    }

    /// <summary>
    /// Stop timing and return elapsed time in milliseconds
    /// </summary>
    /// <returns>Elapsed time in milliseconds</returns>
    public double Stop()
    {
        if (!_isRunning) return 0.0;

        _stopwatch.Stop();
        _isRunning = false;

        return _stopwatch.Elapsed.TotalMilliseconds;
    }

    /// <summary>
    /// Get elapsed time in milliseconds without stopping
    /// </summary>
    /// <returns>Elapsed time in milliseconds</returns>
    public double Elapsed
    {
        get
        {
            if (!_isRunning)
            {
                return _stopwatch.Elapsed.TotalMilliseconds;
            }

            return _stopwatch.Elapsed.TotalMilliseconds;
        }
    }

    /// <summary>
    /// Get elapsed time in microseconds
    /// </summary>
    /// <returns>Elapsed time in microseconds</returns>
    public double ElapsedMicroseconds
    {
        get
        {
            if (!_isRunning)
            {
                return _stopwatch.Elapsed.TotalMicroseconds;
            }

            return _stopwatch.Elapsed.TotalMicroseconds;
        }
    }

    /// <summary>
    /// Reset timer
    /// </summary>
    public void Reset()
    {
        _stopwatch.Reset();
        _isRunning = false;
    }

    /// <summary>
    /// Check if timer is currently running
    /// </summary>
    public bool IsRunning => _isRunning;
}
