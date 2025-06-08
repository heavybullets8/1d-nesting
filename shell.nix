{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  # buildInputs are libraries your code needs to link against
  buildInputs = with pkgs; [
    highs
  ];

  # nativeBuildInputs are the tools you need to build your project
  nativeBuildInputs = with pkgs; [
    gcc
    pkg-config
    gnumake
    # For opening HTML files
    xdg-utils
  ];

  # Set up environment variables to help find HiGHS
  shellHook = ''
    echo "HiGHS development environment loaded"
    echo "pkg-config path: $PKG_CONFIG_PATH"
    echo ""
    echo "Testing HiGHS availability:"
    pkg-config --modversion highs || echo "HiGHS not found via pkg-config"
    echo ""
    echo "To build 64-bit only: make build-64"
    echo "To build both (requires 32-bit libs): make"
  '';
}
