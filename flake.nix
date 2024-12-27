{
  description = "Shell for С.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { nixpkgs, ... }@inputs:
    let
      supportedSystems = [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-linux"
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgs = forAllSystems (system: nixpkgs.legacyPackages.${system});

    in
    {
      devShells = forAllSystems (system: {
        default = 
          pkgs.${system}.mkShellNoCC {
            packages = with pkgs.${system}; [
              gcc
              gdb
              libclang
							astyle
              
              (writeShellScriptBin "r" ''
                gcc "$1" -o file
                shift
                ./file "$@" 
                rm -rf file
              '')
              (writeShellScriptBin "fmt" ''
								astyle -A3 -t8 -p -xg -H -j -xB
							'')
            ];
          };
      });
    };
}
