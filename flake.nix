{
	description = "Universal contest systems terminal interface.";

	inputs = {
		nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

		flake-parts.url = "github:hercules-ci/flake-parts";
	};

	outputs = inputs@{ flake-parts, ... }:
		flake-parts.lib.mkFlake { inherit inputs; } {
			systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin" ];
			perSystem = { config, self', inputs', pkgs, system, lib, ... }: {
				devShells = {
					default = pkgs.mkShell {
						packages = with pkgs; [
							gdb
							gnumake
							libressl
						];
					};
				};

				packages = {
					csti = pkgs.stdenv.mkDerivation rec {
						pname = "csti";
						version = "1.0";
						src = ./.;
						
						buildInputs = [ pkgs.libressl ];
						
						makeFlags = 
							[
								"PREFIX=$(out)"
							];
					};
				};
			};
		};
}
