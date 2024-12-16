{ pkgs, config, ... }:
{
	options = {
		services.notifications-server = {
			enable = pkgs.lib.mkOption {
				type = pkgs.lib.types.bool;
				default = false;
				description = "Enable notifications server";
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
				script = "${pkgs.notifications-server}/bin/notifications_server";
				wantedBy = [ "multi-user.target" ];
				after = [ "network-online.target" ];
			};
		};
	};
}
