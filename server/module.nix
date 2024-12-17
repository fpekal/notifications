{ pkgs, config, ... }:
{
	options = {
		services.notifications-server = {
			enable = pkgs.lib.mkOption {
				type = pkgs.lib.types.bool;
				default = false;
				description = "Enable notifications server";
			};

			auth-key = pkgs.lib.mkOption {
				type = pkgs.lib.types.str;
				default = "secret";
				description = "Authentication key for API";
			};
		};
	};

	config =
	let
		cfg = config.services.notifications-server;
	in {
		systemd.services = pkgs.lib.mkIf cfg.enable {
			notifications-server = {
				enable = true;
				script = "AUTH_KEY=${cfg.auth-key} ${pkgs.notifications-server}/bin/notifications_server";
				wantedBy = [ "multi-user.target" ];
				after = [ "network-online.target" ];
			};
		};
	};
}
