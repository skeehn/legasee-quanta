#!/bin/bash

# Markdown Viewer Script
# Usage: ./view_md.sh [filename.md]

# Check if glow is installed
if ! command -v glow &> /dev/null; then
    echo "❌ Error: glow is not installed."
    echo "Install it with: brew install glow"
    exit 1
fi

# Function to display available markdown files
show_available_files() {
    echo "📚 Available Markdown files:"
    echo "================================"
    find . -name "*.md" -type f | while read -r file; do
        echo "• $file"
    done
    echo ""
    echo "Usage: ./view_md.sh [filename.md]"
    echo "Example: ./view_md.sh README.md"
}

# If no argument provided, show available files
if [ $# -eq 0 ]; then
    show_available_files
    exit 0
fi

# Check if file exists
if [ ! -f "$1" ]; then
    echo "❌ Error: File '$1' not found."
    echo ""
    show_available_files
    exit 1
fi

# Check if file is a markdown file
if [[ ! "$1" == *.md ]]; then
    echo "❌ Error: '$1' is not a markdown file (.md extension required)."
    exit 1
fi

# Display the markdown file with glow
echo "📖 Viewing: $1"
echo "================================"
glow "$1"
