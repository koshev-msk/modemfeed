'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require ui';

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('pollmydevice', _('PollMyDevice'));
		m.description = _('TCP to RS232/RS485 converter');

		s = m.section(form.GridSection, 'interface', _('Settings'));		
		s.tab('general', _('Mode Settings'));
		s.tab('serial', _('Port Settings'));
		s.addremove = true;
		s.nodescriptions = true;
		s.addbtntitle = _('Add new configuration...');
		s.handleAdd = function(ev,name) {
			for (var i = 0; i < 7; i++) {
				var section = uci.get('pollmydevice', String(i));
				if (section == null) {
					var section_id = String(i);
					uci.add('pollmydevice', 'interface', section_id);
					uci.set('pollmydevice', section_id, 'desc',name);
					uci.set('pollmydevice', section_id, 'devicename','/dev/com1');
					uci.set('pollmydevice', section_id, 'mode','disabled');
					this.addedSection = section_id;
					return this.renderMoreOptionsModal(section_id);
				};
			};
			ui.addNotification(null, E('p', 'Exceeded the maximum number of configurations.'), 'error');
		};

		o = s.taboption('general', form.Value, 'desc', _('Description'));

		o = s.taboption('serial', form.Value, 'devicename', _('Port'));
		o.rmempty = false;
		o.value('/dev/com0');
		o.value('/dev/com1');
		o.textvalue = function(section_id) {
			return uci.get('pollmydevice', section_id, 'devicename');
		}

		o = s.taboption('general', form.ListValue, 'mode', _('Mode'));
		o.default = 'disabled';
		o.value(_('disabled'));
		o.value(_('server'));
		o.value(_('client'));

		o = s.taboption('general', form.Flag, 'quiet',_('Disable log messages'));
		o.default = 0;
		o.modalonly = true;
		o.depends({mode: 'disabled', '!reverse': true});

		o = s.taboption('general', form.Value, 'server_port', _('Server Port'));
		o.modalonly = true;
		o.datatype = 'and(uinteger, min(1025), max(65535))';
		o.depends({mode: 'server'});

		o = s.taboption('general', form.ListValue, 'connection_mode', _('Connection Mode'));
		o.modalonly = true;
		o.default = 0;
		o.value(0,_('Alternating'));
		o.value(1,_('Simultaneous'));
		o.depends({mode: 'server'});

		o = s.taboption('general', form.Value, 'holdconntime', _('Connection Hold Time (sec)'));
		o.default = 60;
		o.datatype = 'and(uinteger, min(0), max(100000))'
		o.depends({connection_mode: '0'});
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'pack_size', _('Minimum packet size [0-255] (byte)'));
		o.modalonly = true;
		o.default = 0;
		o.datatype = 'and(uinteger, min(0), max(255))';
		o.depends({mode: 'disabled', '!reverse': true});

		o = s.taboption('general', form.Value, 'pack_timeout', _('Packet timeout [0-255] (x100ms)'));
		o.modalonly = true;
		o.default = 0;
		o.datatype = 'and(uinteger, min(0), max(255))';
		for(i=1;i<256;i++){
			o.depends({pack_size: i.toString()});
		}

		o = s.taboption('general', form.Value, 'client_host', _('Server Host or IP Address'));
		o.default = 'hub.m2m24.ru';
		o.datatype = 'string';
		o.depends({mode: 'client'});
		o.textvalue = function(section_id) {
			if(uci.get('pollmydevice', section_id, 'mode') === 'client') {
				return uci.get('pollmydevice', section_id, 'client_host');
			} else {
				return '-'
			}
		};

		o = s.taboption('general', form.DummyValue, 'port', _('Port'));
		o.modalonly = false;
		o.textvalue = function(section_id) {
			if(uci.get('pollmydevice', section_id, 'client_port')) return uci.get('pollmydevice', section_id, 'client_port');
			if(uci.get('pollmydevice', section_id, 'server_port')) return uci.get('pollmydevice', section_id, 'server_port');
		}

		o = s.taboption('general', form.Value, 'client_port', _('Server Port'));
		o.modalonly = true;
		o.default = 6008;
		o.datatype = 'and(uinteger, min(1025), max(65535))';
		o.depends({mode: 'client'});

		o = s.taboption('general', form.Value, 'client_timeout', _('Client Reconnection Timeout (sec)'));
		o.modalonly = true;
		o.default = 60;
		o.datatype = 'and(uinteger, min(0), max(100000))';
		o.depends({mode: 'client'});

		o = s.taboption('general', form.ListValue, 'client_auth', _('Client Authentification'));
		o.widget='radio';
		o.default = 0;
		o.value(0,_('Disable'));
		o.value(1,_('Enable'));
		o.depends({mode: 'client'});
		o.textvalue = function(section_id) {
			return this.cfgvalue(section_id)==1 ? 'enabled' : 'disabled';
		};

		o = s.taboption('general', form.DummyValue, 'client_id', _('Client ID'));
		o.modalonly = true;
		o.depends({mode: 'client'});

		o = s.taboption('general', form.ListValue, 'modbus_gateway', _('Modbus TCP/IP'));
		o.default = 0
		o.value(0,_('Disabled'));
		o.value(1,_('RTU'));
		o.value(2,_('ASCII'));
		o.depends({mode: 'server'});
		o.depends({client_auth: '0'});
		o.textvalue = function(section_id) {
			if(this.cfgvalue(section_id)==0) return 'disabled';
			if(this.cfgvalue(section_id)==1) return 'RTU';
			if(this.cfgvalue(section_id)==2) return 'ASCII';
		};

		o = s.taboption('serial', form.ListValue, 'baudrate', _('BaudRate'));
		o.default = 9600;
		o.value(300);
		o.value(600);
		o.value(1200);
		o.value(2400);
		o.value(4800);
		o.value(9600);
		o.value(19200);
		o.value(38400);
		o.value(57600);
		o.value(115200);
		o.value(230400);
		o.value(460800);
		o.value(921600);
		o.datatype = 'uinteger';
		o.modalonly = true;
		o.depends({mode: 'disabled', '!reverse': true});

		o = s.taboption('serial', form.ListValue, 'bytesize', _('ByteSize'));
		o.default = 8;
		o.value(5);
		o.value(6);
		o.value(7);
		o.value(8);
		o.datatype = 'uinteger';
		o.modalonly = true;
		o.depends({mode: 'disabled', '!reverse': true});

		o = s.taboption('serial', form.ListValue, 'stopbits', _('StopBits'));
		o.default = 1;
		o.value(1);
		o.value(2);
		o.datatype = 'uinteger';
		o.modalonly = true;
		o.depends({mode: 'disabled', '!reverse': true});

		o = s.taboption('serial', form.ListValue, 'parity', _('Parity'));
		o.default = 'none';
		o.value('even');
		o.value('odd');
		o.value('none');
		o.datatype = 'string';
		o.modalonly = true;
		o.depends({mode: 'disabled', '!reverse': true});

		o = s.taboption('serial', form.ListValue, 'flowcontrol', _('Flow Control'));
		o.modalonly = true;
		o.default = 'none';
		o.value('XON/XOFF');
		o.value('RTS/CTS');
		o.value('none');
		o.datatype = 'string';
		o.depends({mode: 'disabled', '!reverse': true});

		return m.render();
	}
});
