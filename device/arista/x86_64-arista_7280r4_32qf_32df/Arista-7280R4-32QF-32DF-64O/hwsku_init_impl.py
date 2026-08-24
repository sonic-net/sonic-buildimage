from hwsku_init_lib import GearboxPortConfig, PhyPortConfig, PortSpeed, HwSKUInitBase

class HwSKUInit(HwSKUInitBase):
    def generate_gearbox_config(self):
        gearbox_config = self.hwsku_config.get('gearbox_config')
        if not gearbox_config:
            # This SKU does not require to generate gearbox config.
            self.logger.info('Skip generating gearbox config because no config found.')
            return

        # Load speed & lane map for gearbox ports.
        gb_port_speed_lane_map = self.load_sku_json_file(gearbox_config['port_speed_lane_map'])
        if not gb_port_speed_lane_map:
            self.logger.info('Skip generating gearbox config because no port speed lane map found')
            return

        # Generate gearbox and phy port config based on port's configured speed.
        gb_port_config = []
        phy_port_config = {}
        phy_profile = {}
        port_config = self.read_ext_port_config()
        port_names = list(port_config.keys())
        port_names.sort(key=lambda port_name: int(port_name.replace('Ethernet', '')))
        for port in port_names:
            port_speed_lane_map = gb_port_speed_lane_map.get(port)
            if not port_speed_lane_map:
                # No gearbox port for this port.
                continue

            config = port_config[ port ]
            index = int(config['index'])
            speed = PortSpeed(config['speed'])
            phy_id = port_speed_lane_map['phy_id']
            system_lanes = port_speed_lane_map['speed_lane_map'][speed.shortName()]['system_lanes']
            line_lanes = port_speed_lane_map['speed_lane_map'][speed.shortName()]['line_lanes']
            gb_port_config.append(GearboxPortConfig(port, index, phy_id, system_lanes, line_lanes))

            phy_port_config_list = phy_port_config.setdefault(phy_id, [])
            system_speed = speed.value() //len(system_lanes)
            line_speed = speed.value() //len(line_lanes)
            phy_port_config_list.append(PhyPortConfig(index, system_speed, line_speed))

            profile = phy_profile.setdefault(phy_id, {})
            if speed.shortName() == '400G':
                profile['topology'] = 2
                profile['mode'] = 'bitmux'
            else:
                profile['topology'] = 1
                profile['mode'] = 'retimer'

        # Render templates to generate gearbox and phy config files.
        self.render_template(gearbox_config['template'], gearbox_config['template_data'], gb_port_config,
                             gearbox_config['file'])
        self.logger.info('Generated gearbox config: {}'.format(gearbox_config['file']))
        phy_config = self.hwsku_config['phy_config']
        for phy_id in sorted(phy_port_config.keys()):
            self.render_template(phy_config['template'], phy_config['template_data'], phy_port_config[ phy_id ],
                                 phy_config['file'].format(phy_id))
            self.logger.info('Generated phy config: {}'.format(phy_config['file'].format(phy_id)))
        phy_profile_config = self.hwsku_config['phy_profile_config']
        self.render_template(phy_profile_config['template'], phy_profile_config['template_data'], phy_profile,
                             phy_profile_config['file'])
        self.logger.info('Generated phy profile: {}'.format(phy_profile_config['file']))

    def generate_bcm_config(self):
        sku_bcm_config = self.hwsku_config.get('bcm_config')
        if not sku_bcm_config:
            # This SKU does not require to generate bcm config.
            self.logger.info('Skip generating bcm config because no config found.')
            return

        bcm_port_lane_map = self.load_sku_json_file(sku_bcm_config['port_lane_map'])
        port_config = self.read_ext_port_config()
        bcm_port_lane_config = { 'port_map' : {},
                                 'speed_lane_map' : {},
                                 'lane_to_serdes_map' : {},
                                 'phy_polarity_flip' : {} }
        for port in port_config:
            config = port_config[port]
            index = int(config['index'])
            bcm_port_lane_config['port_map'][index] = {'core_id': config['core_id'],
                                                       'core_port_id': config['core_port_id'] }
            speed = PortSpeed(config['speed'])
            bcm_port_lane_config['speed_lane_map'][index] = bcm_port_lane_map['port_speed_lane_map'][str(index)][speed.shortName()]

            for lane in config['lanes'].split(','):
                lane = int(lane)
                bcm_port_lane_config['lane_to_serdes_map'][lane] = bcm_port_lane_map['lane_to_serdes_map'][str(lane)][speed.shortName()]
                bcm_port_lane_config['phy_polarity_flip'][lane] = bcm_port_lane_map['phy_polarity_flip'][str(lane)][speed.shortName()]

        # Render templates to generate bcm config
        self.render_template(sku_bcm_config['template'], sku_bcm_config['template_data'], bcm_port_lane_config,
                             sku_bcm_config['file'])
        self.logger.info('Generated bcm config: {}'.format(sku_bcm_config['file']))

    def do_init(self):
        if self.hwsku_config is None:
            # The SKU does not require any runtime initialization.
            self.logger.info('Skip hwsku-init because no hwsku config found.')
            return
   
        self.generate_gearbox_config()
        self.generate_bcm_config()
