{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "1d-nesting-dev";

  buildInputs = with pkgs; [
    # Build tools
    gcc
    gnumake
    pkg-config
    bear  # For generating compile_commands.json

    # Dependencies
    highs

    # Development tools
    clang-tools  # Provides clang-format, clang-tidy, clangd (LSP)

    # Utilities
    wget
  ];

  shellHook = ''
    echo "1D Nesting Development Environment"
    echo "=================================="
    echo "HiGHS: ${pkgs.highs}"
    echo ""
    echo "Available commands:"
    echo "  make web    - Build web server"
    echo "  make clean  - Clean build artifacts"
    echo ""

    # Set up environment for LSP/IDE
    export HIGHS_INSTALL_PATH="${pkgs.highs}"

    # Generate compile_commands.json for LSP if not present
    if [ ! -f compile_commands.json ]; then
      echo "📝 Generating compile_commands.json for LSP..."
      make clean
      bear -- make web
      echo "✅ compile_commands.json generated"
    fi
  '';
}
