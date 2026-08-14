'use strict';
'require form';
'require uci';
'require view';
'require dom';
'require modemmanager_helper as helper';

function getBandInfo(band) {
    var match = /^(utran|eutran|ngran)-(\d+)$/.exec(band);

    if (match) {
        var generation = {
            'utran':  { key: '3g', title: _('3G / UMTS'), order: 2 },
            'eutran': { key: '4g', title: _('4G / LTE'), order: 3 },
            'ngran':  { key: '5g', title: _('5G / NR'), order: 4 }
        }[match[1]];

        return {
            value: band,
            number: parseInt(match[2], 10),
            label: match[1] == 'ngran' ? 'n' + match[2] : 'B' + match[2],
            generation: generation
        };
    }

    return {
        value: band,
        number: null,
        label: band,
        generation: {
            key: '2g',
            title: _('2G / GSM'),
            order: 1
        }
    };
}

function groupBands(bands) {
    var groups = {};

    (bands || []).forEach(function(band) {
        var info = getBandInfo(band);
        var key = info.generation.key;

        if (!groups[key]) {
            groups[key] = {
                key: key,
                title: info.generation.title,
                order: info.generation.order,
                bands: []
            };
        }

        groups[key].bands.push(info);
    });

    Object.keys(groups).forEach(function(key) {
        groups[key].bands.sort(function(a, b) {
            if (a.number !== null && b.number !== null)
                return a.number - b.number;

            if (a.number !== null)
                return -1;

            if (b.number !== null)
                return 1;

            return a.value.localeCompare(b.value);
        });
    });

    return Object.keys(groups).map(function(key) {
        return groups[key];
    }).sort(function(a, b) {
        return a.order - b.order;
    });
}

var BandValue = form.Value.extend({
    supportedBands: [],
    selectedBands: [],

    formvalue: function(section_id) {
        var node = document.getElementById(this.cbid(section_id));
        if (!node)
            return [];

        return Array.prototype.slice.call(
            node.querySelectorAll('input[type="checkbox"][data-band-value]:checked')
        ).map(function(input) {
            return input.getAttribute('data-band-value');
        });
    },

    renderWidget: function(section_id) {
        var container = E('div', {
            'class': 'mmconfig-bands',
            'id': this.cbid(section_id)
        });

        var selected = {};
        (this.selectedBands || []).forEach(function(band) {
            selected[band] = true;
        });

        var groups = groupBands(this.supportedBands);

        if (!groups.length) {
            return E('div', { 'class': 'mmconfig-bands-empty' },
                _('No supported bands reported by ModemManager.'));
        }

        groups.forEach(function(group) {
            var groupNode = E('div', { 'class': 'mmconfig-band-group' });
            var header = E('div', { 'class': 'mmconfig-band-group-header cbi-rowstyle-2' });
            var title = E('strong', { 'class': 'mmconfig-band-group-title' }, group.title);
            var actions = E('span', { 'class': 'mmconfig-band-actions' });

            actions.appendChild(E('button', {
                'type': 'button',
                'class': 'btn cbi-button cbi-button-neutral mmconfig-band-action',
                'click': function() {
                    groupNode.querySelectorAll('input[type="checkbox"][data-band-value]').forEach(function(input) {
                        input.checked = true;
                    });
                }
            }, _('All')));

            actions.appendChild(E('button', {
                'type': 'button',
                'class': 'btn cbi-button cbi-button-neutral mmconfig-band-action',
                'click': function() {
                    groupNode.querySelectorAll('input[type="checkbox"][data-band-value]').forEach(function(input) {
                        input.checked = false;
                    });
                }
            }, _('None')));

            header.appendChild(title);
            header.appendChild(actions);

            var grid = E('div', { 'class': 'mmconfig-band-grid' });

            group.bands.forEach(function(band) {
                var id = this.cbid(section_id) + '-' +
                    band.value.replace(/[^a-zA-Z0-9_-]/g, '-');

                var input = E('input', {
                    'type': 'checkbox',
                    'id': id,
                    'data-band-value': band.value
                });

                input.checked = !!selected[band.value];

                grid.appendChild(E('label', {
                    'class': 'mmconfig-band-item',
                    'for': id,
                    'title': band.value
                }, [
                    input,
                    E('span', { 'class': 'mmconfig-band-label' }, band.label)
                ]));
            }, this);

            groupNode.appendChild(header);
            groupNode.appendChild(grid);
            container.appendChild(groupNode);
        }, this);

        return container;
    }
});


return view.extend({
    load: function() {
        return Promise.all([
            uci.load('mmconfig'),
            helper.getModems()
        ]);
    },

    render: function(data) {
        var modemsData = data[1];
        var m = new form.Map('mmconfig', _('Modem Configuration'), _('Select bands for modem operation.') +
			'<br />' + _('Selected bands are a recommendation and do not guarantee that the modem will use exactly these bands.') +
			'<br />' + _('If all bands are deselected, the modems default band configuration will be used.'));
        
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
                
                html += '<div class="compact-line cbi-rowstyle-2">';
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
                o = s.option(BandValue, 'bands', _('Bands'));

                o.supportedBands = modemObj.generic['supported-bands'].slice();

                var currentBands = modemObj.generic['current-bands'] || [];
                var currentSet = {};
                currentBands.forEach(function(band) {
                    currentSet[band] = true;
                });

                // Mark a band as active only when ModemManager reports it
                // in current-bands. Only supported bands are displayed.
                o.selectedBands = o.supportedBands.filter(function(band) {
                    return !!currentSet[band];
                });
                o.rmempty = true;
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
            '  border: 1px solid #e2e8f0;',
            '  border-radius: 6px;',
            '  //padding: 12px 16px;',
	    '  padding: 2px 2px;',
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
            '  //color: #2d3748;',
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
            '.mmconfig-bands {',
            '  border: 1px solid #e2e8f0;',
            '  border-radius: 6px;',
            '  overflow: hidden;',
            '  margin-top: 4px;',
            '}',
            '',
            '.mmconfig-band-group + .mmconfig-band-group {',
            '  border-top: 1px solid #e2e8f0;',
            '}',
            '',
            '.mmconfig-band-group-header {',
            '  display: flex;',
            '  align-items: center;',
            '  justify-content: space-between;',
            '  gap: 10px;',
            '  padding: 8px 12px;',
            '  //background: #f8fafc;',
            '}',
            '',
            '.mmconfig-band-group-title {',
            '  font-size: 0.95em;',
            '}',
            '',
            '.mmconfig-band-actions {',
            '  display: flex;',
            '  gap: 4px;',
            '}',
            '',
            '.mmconfig-band-action {',
            '  padding: 2px 8px;',
            '  font-size: 0.85em;',
            '}',
            '',
            '.mmconfig-band-grid {',
            '  display: grid;',
            '  grid-template-columns: repeat(auto-fill, minmax(72px, 1fr));',
            '  gap: 6px;',
            '  padding: 10px 12px 12px;',
            '}',
            '',
            '.mmconfig-band-item {',
            '  display: flex;',
            '  align-items: center;',
            '  gap: 6px;',
            '  min-height: 30px;',
            '  padding: 4px 6px;',
            '  border: 1px solid #e2e8f0;',
            '  border-radius: 4px;',
            '  cursor: pointer;',
            '  user-select: none;',
            '}',
            '',
            '.mmconfig-band-item:hover {',
            '  background: #f8fafc;',
            '}',
            '',
            '.mmconfig-band-label {',
            '  font-weight: 500;',
            '}',
            '',
            '.mmconfig-bands-empty {',
            '  padding: 10px 12px;',
            '  border: 1px solid #e2e8f0;',
            '  border-radius: 6px;',
            '  color: #718096;',
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
