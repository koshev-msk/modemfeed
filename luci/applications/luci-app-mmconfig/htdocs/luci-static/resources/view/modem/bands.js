'use strict';
'require form';
'require uci';
'require view';
'require dom';
'require modemmanager_helper as helper';

return view.extend({
    load: function() {
        return Promise.all([
            uci.load('mmconfig'),
            helper.getModems()
        ]);
    },

    render: function(data) {
        var modemsData = data[1];
        var m = new form.Map('mmconfig', _('Modem Configuration'), _('List supported bands.<br />If deselect all bands, then used default band modem config.'));
        
        // add styles
        var style = document.createElement('style');
        style.textContent = this.getCSS();
        document.head.appendChild(style);
        
        var configSections = [];
        uci.sections('mmconfig', 'modem', function(s) {
            configSections.push(s);
        });
        
        configSections.forEach(function(section, index) {
            // search modems
            var modemObj = null;
            if (modemsData && modemsData.length > 0) {
                for (var i = 0; i < modemsData.length; i++) {
                    if (modemsData[i] && 
                        modemsData[i].modem && 
                        modemsData[i].modem.generic && 
                        modemsData[i].modem.generic.device === section.device) {
                        modemObj = modemsData[i].modem;
                        break;
                    }
                }
            }
            
            // create sections
            var s = m.section(form.NamedSection, section['.name'], _('Modem ') + (index + 1));
            s.addremove = false;
            s.anonymous = false;
            
            // hide device option
            var o = s.option(form.HiddenValue, 'device', '');
            o.default = section.device || '';
            
            // container
            if (modemObj && modemObj.generic) {
                var infoPanel = s.option(form.DummyValue, '_info_panel', '');
                infoPanel.rawhtml = true;
                
                var html = '<div class="modem-info-compact">';
                
                // Operator name modem and access tech 
                var modelText = '';
                if (modemObj.generic.manufacturer || modemObj.generic.model) {
                    if (modemObj.generic.manufacturer) {
                        modelText += modemObj.generic.manufacturer + ' ';
                    }
                    if (modemObj.generic.model) {
                        modelText += modemObj.generic.model;
                    }
                } else {
                    modelText = _('Unknown modem');
                }
                
                var operatorText = '';
                if (modemObj['3gpp'] && modemObj['3gpp']['operator-name']) {
                    operatorText = modemObj['3gpp']['operator-name'];
                }
                
                html += '<div class="compact-line">';
                html += '<span class="modem-model">' + modelText + '</span>';
                
                if (operatorText) {
                    html += '<span class="separator">•</span>';
                    html += '<span class="modem-operator">' + operatorText + '</span>';
                }


               var currentModeText = '';
               if (modemObj.generic['current-modes']) {

                    currentModeText = modemObj.generic['current-modes'];
               } 
               // if current-modes not aviaible, use access-technologies
               else if (modemObj.generic['access-technologies'] && modemObj.generic['access-technologies'].length > 0) {
              // Берем первую технологию из access-technologies
                   currentModeText = modemObj.generic['access-technologies'];
              }

              if (currentModeText) {
                 html += '<span class="separator">•</span>';
	      html += '<span class="current-mode">' + _('Access Tech:') + ' ' + currentModeText + '</span>';
            }

                html += '</div>';
                
                html += '</div>'; // close container
                
                infoPanel.default = html;
            }
            
            // network preffered
            o = s.option(form.ListValue, 'preffer', _('Network Mode'));
            o.rmempty = false;
            
            if (modemObj && modemObj.generic && modemObj.generic['supported-modes']) {
                // get from modem supported-modes 
                modemObj.generic['supported-modes'].forEach(function(mode) {
                    o.value(mode, mode);
                });

                // Set current
              if (section.preffer) {
                   o.default = section.preffer;
              } else if (currentModeText && modemObj.generic['supported-modes'].includes(currentModeText)) {
                   o.default = currentModeText;
	      }
            } else {
                // If not aviable
                o.value('', _('Not Available'));
		o.default = '';
		o.readonly = true;
            }

            // bands select
            if (modemObj && modemObj.generic && modemObj.generic['supported-bands']) {
                o = s.option(form.MultiValue, 'bands', _('Bands'));

                // get from modem supported-bands
                modemObj.generic['supported-bands'].forEach(function(band) {
                    o.value(band, band);
                });

                // Set current
                if (section.bands) {
                    o.default = section.bands;
		}
            } else {
		o = s.option(form.Value, 'bands', _('Bands'));
		o.value('', _('Not Available'));
		o.default = '';
		o.readonly = true;
	    }
            
            // small separator
            if (index < configSections.length - 1) {
                var spacer = s.option(form.DummyValue, '_divider', '');
                spacer.default = '<div class="light-divider"></div>';
                spacer.rawhtml = true;
            }
        });
        
        if (configSections.length === 0) {
            var s = m.section(form.NamedSection, 'info', _('WARNING'));
            s.anonymous = true;
            
            var o = s.option(form.DummyValue, '_message', _('Status'));
            o.default = _('No modem configuration found. Run <code>/etc/init.d/mmconfig start<code>');
            o.rawhtml = false;
        }
        
        return m.render();
    },
    
    getCSS: function() {
        return [
            '.modem-info-compact {',
            '  background: #f8fafc;',
            '  border: 1px solid #e2e8f0;',
            '  border-radius: 6px;',
            '  padding: 12px 16px;',
            '  margin: 15px 0;',
            '}',
            '',
            '.compact-line {',
            '  display: flex;',
            '  align-items: center;',
            '  gap: 10px;',
            '}',
            '',
            '.modem-model {',
            '  font-weight: 600;',
            '  color: #2d3748;',
            '  font-size: 1em;',
            '}',
            '',
            '.separator {',
            '  color: #a0aec0;',
            '  font-weight: 300;',
            '}',
            '',
            '.modem-operator {',
            '  color: #4a5568;',
            '  font-size: 0.95em;',
            '}',
            '',
            '.light-divider {',
            '  height: 1px;',
            '  background: #edf2f7;',
            '  margin: 20px 0;',
            '}',
            '',
            '/* Улучшаем отступы в секциях */',
            '.cbi-section .cbi-section-node {',
            '  margin-bottom: 10px;',
            '}',
            '',
            '.cbi-section .cbi-section-descr {',
            '  padding: 5px 0;',
            '}'
        ].join('\n');
    }
});
