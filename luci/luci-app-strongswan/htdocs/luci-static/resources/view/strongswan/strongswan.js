'use strict';
'require fs';
'require rpc';
'require uci';
'require view';
'require form';
'require tools.widgets as widgets';

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('ipsec', _('IPsec (Internet Protocol Security)'));
		m.description = _('strongSwan IPsec Configuration');

		s = m.section(form.GridSection, 'conn', _('Settings'));
		s.tab('general', _('General Settings'));
		s.tab('phase1', _('Phase 1'));
		s.tab('phase2', _('Phase 2'));
		s.addremove = true;
		s.nodescriptions = true;

		o = s.taboption('general', form.Flag, 'enabled', _('Enabled'));
		o.rmempty = false;

		o = s.taboption('general', form.ListValue, 'keyexchange', _('IKE version'), _('Method of key exchange'));
		o.default = 'ikev2';
		o.value('ikev1',_('IKEv1'));
		o.value('ikev2',_('IKEv2'));
		

		o = s.taboption('general', form.ListValue, 'aggressive', _('Mode'), _('ISAKMP (Internet Security Association and Key Management Protocol) phase 1 exchange mode'));
		o.default = 'no';
		o.value('no',_('Main'));
		o.value('yes',_('Aggressive'));
		o.depends({keyexchange: 'ikev1'});
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'psk_key', _('Preshared key'), _('A shared password to authenticate between the peers'));
		o.rmempty = false;
		o.modalonly = true;

		o = s.taboption('general', form.ListValue, 'leftidtype', _('Local identifier type'), _('Choose one accordingly to your IPSec configuration'));
		o.default = 'address';
		o.value('address',_('Address'));
		o.value('fqdn',_('FQDN'));
		o.value('user_fqdn',_('User FQDN'));
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'leftid', _('Local identifier'), _('Set the device identifier for IPSec tunnel'));
		o.rmempty = false;
		o.modalonly = true;

		o = s.taboption('general', form.DynamicList, 'leftsubnet', _('Local IP address/Subnet mask'));
		o.placeholder = '192.168.88.0/24'
		o.datatype = 'ipaddr'
		o.rmempty = false;
		o.modalonly = true;

		o = s.taboption('general', form.ListValue, 'rightidtype', _('Remote identifier type'), _('Choose one accordingly to your IPSec configuration'));
		o.default = 'address';
		o.value('address',_('Address'));
		o.value('fqdn',_('FQDN'));
		o.value('user_fqdn',_('User FQDN'));
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'rightid', _('Remote identifier'), _('Set the remote identifier for IPSec tunnel'));
		o.rmempty = false;
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'right', _('Remote VPN endpoint'), _('Domain name or IP address. Leave empty for any'));
		o.datatype = 'host'
		o.textvalue = function(section_id) {
			if(!uci.get('ipsec', section_id, 'right')) {
				return _('any')
			} else {
				return uci.get('ipsec', section_id, 'right')
			}
		};

		o = s.taboption('general', form.DynamicList, 'rightsubnet', _('Remote IP address/Subnet mask'), _('Should differ from local IP address/Subnet mask'));
		o.placeholder = '192.168.100.0/24'
		o.datatype = 'ipaddr'
		o.rmempty = false;

		o = s.taboption('general', form.ListValue, 'dpdaction', _('Dead Peer Detection action'));
		o.default = 'restart';
		o.value('none');
		o.value('restart');
		o.rmempty = false;
		
		o = s.taboption('general', form.Value, 'dpddelay', _('DPD delay (sec)'), _('Delay between peer acknowledgement requests'));
		o.default = '30';
		o.datatype = 'and(uinteger, min(0))'
		o.rmempty = false;
		o.modalonly = true;
		o.depends({dpdaction: 'restart'});

		o = s.taboption('phase1', form.ListValue, 'ike_encryption_algorithm', _('Encryption algorithm'), _('The encryption algorithm must match with another incoming connection to establish IPSec'));
		o.default = '3des';
		o.value('des',_('DES'));
		o.value('3des',_('3DES'));
		o.value('aes128',_('AES128'));
		o.value('aes192',_('AES192'));
		o.value('aes256',_('AES256'));
		o.modalonly = true;

		o = s.taboption('phase1', form.ListValue, 'ike_authentication_algorithm', _('Authentication algorithm'), _('The authentication algorithm must match with another incoming connection to establish IPSec'));
		o.default = 'sha1';
		o.value('md5',_('MD5'));
		o.value('sha1',_('SHA1'));
		o.value('sha256',_('SHA256'));
		o.value('sha384',_('SHA384'));
		o.value('sha512',_('SHA512'));
		o.modalonly = true;

		o = s.taboption('phase1', form.ListValue, 'ike_dh_group', _('DH group'), _('The DH (Diffie-Hellman) group must match with another incoming connection to establish IPSec'));
		o.default = 'modp1536';
		o.value('modp768',_('MODP768'));
		o.value('modp1024',_('MODP1024'));
		o.value('modp1536',_('MODP1536'));
		o.value('modp2048',_('MODP2048'));
		o.value('modp3072',_('MODP3072'));
		o.value('modp4096',_('MODP4096'));
		o.modalonly = true;

		o = s.taboption('phase1', form.Value, 'ikelifetime', _('Lifetime'), _('The time duration for phase 1'));
		o.datatype = 'and(uinteger, min(0))'
		o.default = '8';
		o.modalonly = true;

		o = s.taboption('phase1', form.ListValue, 'ikeletter', _('in'));
		o.default = 'h';
		o.value('h',_('hours'));
		o.value('m',_('minutes'));
		o.value('s',_('seconds'));
		o.modalonly = true;

		o = s.taboption('phase2', form.ListValue, 'esp_encryption_algorithm', _('Encryption algorithm'), _('The encryption algorithm must match with another incoming connection to establish IPSec'));
		o.default = '3des';
		o.value('des',_('DES'));
		o.value('3des',_('3DES'));
		o.value('aes128',_('AES128'));
		o.value('aes192',_('AES192'));
		o.value('aes256',_('AES256'));
		o.modalonly = true;

		o = s.taboption('phase2', form.ListValue, 'esp_hash_algorithm', _('Hash algorithm'), _('The hash algorithm must match with another incoming connection to establish IPSec'));
		o.default = 'sha1';
		o.value('md5',_('MD5'));
		o.value('sha1',_('SHA1'));
		o.value('sha256',_('SHA256'));
		o.value('sha384',_('SHA384'));
		o.value('sha512',_('SHA512'));
		o.modalonly = true;

		o = s.taboption('phase2', form.ListValue, 'esp_pfs_group', _('PFS group'), _('The PFS (Perfect Forward Secrecy) group must match with another incoming connection to establish IPSec'));
		o.default = 'modp1536';
		o.value('modp768',_('MODP768'));
		o.value('modp1024',_('MODP1024'));
		o.value('modp1536',_('MODP1536'));
		o.value('modp2048',_('MODP2048'));
		o.value('modp3072',_('MODP3072'));
		o.value('modp4096',_('MODP4096'));
		o.value('no_pfs',_('No PFS'));
		o.modalonly = true;

		o = s.taboption('phase2', form.Value, 'keylife', _('Lifetime'), _('The time duration for phase 2'));
		o.datatype = 'and(uinteger, min(0))'
		o.default = '8';
		o.modalonly = true;

		o = s.taboption('phase2', form.ListValue, 'letter', _('in'));
		o.default = 'h';
		o.value('h',_('hours'));
		o.value('m',_('minutes'));
		o.value('s',_('seconds'));
		o.modalonly = true;

		return m.render();
	}
});
