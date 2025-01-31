{
  description = "Universal contest systems terminal interface.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix.url = "github:numtide/treefmt-nix";
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
        inputs.treefmt-nix.flakeModule
      ];
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];
      perSystem =
        { pkgs, ... }:
        {
          devShells = {
            default = pkgs.mkShell {
              packages = with pkgs; [
                gdb
                gnumake
                libressl
              ];
            };
          };

          treefmt = {
            projectRootFile = "flake.nix";

            programs.clang-format.enable = true;
            programs.deadnix.enable = true;
            programs.statix.enable = true;
            programs.nixfmt.enable = true;
          };

          packages = {
            csti = pkgs.stdenv.mkDerivation rec {
              pname = "csti";
              version = "1.0";
              src = ./.;

              buildInputs = [ pkgs.libressl ];

              makeFlags = [
                "PREFIX=$(out)"
              ];
            };
          };
        };
    };
}
