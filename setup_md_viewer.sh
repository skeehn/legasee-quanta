#!/bin/bash

# Setup script for Markdown viewing in terminal
# Run this script to configure your shell for better Markdown viewing

echo "🔧 Setting up Markdown viewer for terminal..."

# Check if glow is installed
if ! command -v glow &> /dev/null; then
    echo "📦 Installing glow (Markdown viewer)..."
    if command -v brew &> /dev/null; then
        brew install glow
    else
        echo "❌ Error: Homebrew not found. Please install Homebrew first:"
        echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi
fi

# Detect shell
SHELL_CONFIG=""
if [[ "$SHELL" == *"zsh"* ]]; then
    SHELL_CONFIG="$HOME/.zshrc"
elif [[ "$SHELL" == *"bash"* ]]; then
    SHELL_CONFIG="$HOME/.bashrc"
else
    echo "⚠️  Warning: Unknown shell. Please manually add the alias to your shell config."
    exit 1
fi

# Create alias for easy Markdown viewing
ALIAS_LINE='alias md="glow"'
ALIAS_LINE2='alias viewmd="./view_md.sh"'

# Check if alias already exists
if grep -q "alias md=" "$SHELL_CONFIG" 2>/dev/null; then
    echo "✅ Markdown alias already exists in $SHELL_CONFIG"
else
    echo "📝 Adding Markdown alias to $SHELL_CONFIG..."
    echo "" >> "$SHELL_CONFIG"
    echo "# Markdown viewer aliases" >> "$SHELL_CONFIG"
    echo "$ALIAS_LINE" >> "$SHELL_CONFIG"
    echo "$ALIAS_LINE2" >> "$SHELL_CONFIG"
    echo "✅ Added Markdown aliases to $SHELL_CONFIG"
fi

echo ""
echo "🎉 Setup complete! You can now use:"
echo "   • md README.md          # View any Markdown file with glow"
echo "   • viewmd README.md      # Use the project's viewer script"
echo "   • ./view_md.sh          # List all available Markdown files"
echo ""
echo "💡 To apply changes, restart your terminal or run:"
echo "   source $SHELL_CONFIG"
