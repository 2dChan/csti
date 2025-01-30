{
  description = "Shell for С.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs, ... }@inputs:
    let
      supportedSystems = [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-linux"
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgs = forAllSystems (system: nixpkgs.legacyPackages.${system});
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlay ]; });

    in
    {
      overlay = final: prev: {
        csti = with final; stdenv.mkDerivation rec {
          pname = "csti";
          version = "1.0";

          src = ./.;

          buildInputs = [ libressl ];
					
					makeFlags = 
						[
							"PREFIX=$(out)"
						];
        };
      };

      packages = forAllSystems (system:
        {
          inherit (nixpkgsFor.${system}) csti;
        });

      devShells = forAllSystems (system: {
        default = 
          pkgs.${system}.mkShellNoCC {
            packages = with pkgs.${system}; [
              gcc
              gdb
              gnumake
              libressl
              
              clang-tools
            ];
          };
      });
    };
}
