#!/bin/bash
# run_coverage.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if required tools are installed
check_dependencies() {
    print_status "Checking dependencies..."

    # C++ dependencies
    if ! command -v cmake &> /dev/null; then
        print_error "cmake is not installed"
        exit 1
    fi

    if ! command -v gcov &> /dev/null; then
        print_error "gcov is not installed"
        exit 1
    fi

    if ! command -v lcov &> /dev/null; then
        print_error "lcov is not installed. Install with: sudo apt-get install lcov (Ubuntu) or brew install lcov (macOS)"
        exit 1
    fi

    if ! command -v genhtml &> /dev/null; then
        print_error "genhtml is not installed (part of lcov package)"
        exit 1
    fi

    # C# dependencies
    if ! command -v dotnet &> /dev/null; then
        print_error "dotnet is not installed"
        exit 1
    fi

    print_success "All dependencies are available"
}

# Clean previous coverage data
clean_coverage() {
    print_status "Cleaning previous coverage data..."

    # Clean C++ coverage
    if [ -d "build" ]; then
        rm -rf build
    fi

    # Clean C# coverage
    if [ -d "PackPlannerCSharp" ]; then
        find PackPlannerCSharp -name "coverage.*.xml" -delete
        find PackPlannerCSharp -name "TestResults" -type d -exec rm -rf {} + 2>/dev/null || true
    fi

    # Clean coverage reports
    rm -rf coverage_reports
    mkdir -p coverage_reports

    print_success "Coverage data cleaned"
}

# Build and run C++ tests with coverage
run_cpp_coverage() {
    print_status "Running C++ tests with coverage..."

    # Create build directory
    mkdir -p build
    cd build

    # Configure with coverage flags
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0 -g" \
          -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0 -g" \
          -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
          ..

    # Build
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    # Run tests
    print_status "Running C++ unit tests..."
    if ! ctest --output-on-failure; then
        print_error "C++ tests failed"
        cd ..
        return 1
    fi

    # Generate coverage data
    print_status "Generating C++ coverage report..."

    # Create coverage directory
    mkdir -p coverage

    # Capture coverage data
    lcov --capture --directory . --output-file coverage/coverage.info --ignore-errors mismatch

    # Remove system headers and test files from coverage
    lcov --remove coverage/coverage.info \
         '/usr/*' \
         '*/tests/*' \
         '*/test/*' \
         '*/gtest/*' \
         '*/gmock/*' \
         '*/_deps/*' \
         --output-file coverage/coverage_filtered.info

    # Generate HTML report
    genhtml coverage/coverage_filtered.info \
            --output-directory coverage/html \
            --title "Pack Planner C++ Coverage" \
            --num-spaces 4 \
            --legend

    # Copy to main coverage reports directory
    cp -r coverage/html ../coverage_reports/cpp_coverage

    cd ..
    print_success "C++ coverage report generated"
}

# Build and run C# tests with coverage
run_csharp_coverage() {
    print_status "Running C# tests with coverage..."

    if [ ! -d "PackPlannerCSharp" ]; then
        print_warning "PackPlannerCSharp directory not found, skipping C# coverage"
        return 0
    fi

    cd PackPlannerCSharp
    cd PackPlanner.Tests

    # Restore packages
    dotnet restore

    # Install coverage tools if not already installed
    if ! dotnet tool list -g | grep -q coverlet.console; then
        print_status "Installing coverlet.console..."
        dotnet tool install --global coverlet.console
    fi

    if ! dotnet tool list -g | grep -q dotnet-reportgenerator-globaltool; then
        print_status "Installing reportgenerator..."
        dotnet tool install --global dotnet-reportgenerator-globaltool
    fi

    # Build the solution
    dotnet build --configuration Debug

    # Run tests with coverage
    print_status "Running C# unit tests with coverage..."
    dotnet test --configuration Debug \
                --collect:"XPlat Code Coverage" \
                --results-directory TestResults \
                --logger trx \
                --verbosity normal

    # Find the coverage file
    COVERAGE_FILE=$(find TestResults -name "coverage.cobertura.xml" | head -1)

    if [ -z "$COVERAGE_FILE" ]; then
        print_error "Coverage file not found"
        cd ..
        return 1
    fi

    # Generate HTML report
    print_status "Generating C# coverage report..."
    reportgenerator \
        -reports:"$COVERAGE_FILE" \
        -targetdir:"../../coverage_reports/csharp_coverage" \
        -reporttypes:"Html;HtmlSummary" \
        -title:"Pack Planner C# Coverage"

    cd ..
    print_success "C# coverage report generated"
}

# Generate combined coverage summary
generate_summary() {
    print_status "Generating coverage summary..."

    cat > ../coverage_reports/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Pack Planner - Code Coverage Reports</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .section { margin: 20px 0; padding: 20px; border: 1px solid #ddd; border-radius: 5px; }
        .link { display: inline-block; margin: 10px; padding: 10px 20px; background-color: #007cba; color: white; text-decoration: none; border-radius: 3px; }
        .link:hover { background-color: #005a87; }
        .timestamp { color: #666; font-size: 0.9em; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Pack Planner - Code Coverage Reports</h1>
        <p class="timestamp">Generated on: $(date)</p>
    </div>

    <div class="section">
        <h2>C++ Coverage</h2>
        <p>Coverage report for C++ implementation using gcov/lcov</p>
        <a href="cpp_coverage/index.html" class="link">View C++ Coverage Report</a>
    </div>

    <div class="section">
        <h2>C# Coverage</h2>
        <p>Coverage report for C# implementation using coverlet/reportgenerator</p>
        <a href="csharp_coverage/index.html" class="link">View C# Coverage Report</a>
    </div>

    <div class="section">
        <h2>Instructions</h2>
        <p>Click on the links above to view detailed coverage reports for each implementation.</p>
        <p>The reports show line-by-line coverage information and summary statistics.</p>
    </div>
</body>
</html>
EOF

    print_success "Coverage summary generated at coverage_reports/index.html"
}

# Main execution
main() {
    print_status "Starting code coverage analysis for Pack Planner..."

    # Check dependencies
    check_dependencies

    # Clean previous data
    clean_coverage

    # Run C++ coverage
    if ! run_cpp_coverage; then
        print_error "C++ coverage failed"
        exit 1
    fi

    # Run C# coverage
    if ! run_csharp_coverage; then
        print_error "C# coverage failed"
        exit 1
    fi

    # Generate summary
    generate_summary

    print_success "Code coverage analysis completed!"
    print_status "Open coverage_reports/index.html in your browser to view the results"

    # Try to open the report automatically
    if command -v xdg-open &> /dev/null; then
        xdg-open ../coverage_reports/index.html
    elif command -v open &> /dev/null; then
        open ../coverage_reports/index.html
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --clean        Clean coverage data only"
        echo "  --cpp-only     Run only C++ coverage"
        echo "  --csharp-only  Run only C# coverage"
        exit 0
        ;;
    --clean)
        clean_coverage
        exit 0
        ;;
    --cpp-only)
        check_dependencies
        clean_coverage
        run_cpp_coverage
        exit 0
        ;;
    --csharp-only)
        check_dependencies
        clean_coverage
        run_csharp_coverage
        exit 0
        ;;
    "")
        main
        ;;
    *)
        print_error "Unknown option: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac
