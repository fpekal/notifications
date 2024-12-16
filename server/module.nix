{ pkgs, config, ... }:
{
	options = {
		services.notifications-server = {
			enabled = pkgs.lib.mkOption {
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
		systemd.services = pkgs.lib.mkIf cfg.enabled {
			notifications-server = {
				enabled = true;
				script = "${pkgs.notifications-server}/bin/notifications-server";
				wanterBy = [ "multi-user.target" ];
				after = [ "network-online.target" ];
			};
		};
	};
}
