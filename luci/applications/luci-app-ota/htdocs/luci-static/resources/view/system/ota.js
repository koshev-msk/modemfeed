'use strict';
'require dom';
'require fs';
'require ui';
'require view';

/*
 * OTA Update Application
 */

return view.extend({
    // Format raw firmware filename → "OpenWrt 24.10.7 202606-rev01"
    fmtVer: function(raw) {
        if (!raw) return '';
        // From openwrt-24.10.7-4g-lte-202606-rev01
        var m = raw.match(/^openwrt-([\d.]+)-.*-(\d{6}-rev\d+)$/);
        if (m) return 'OpenWrt ' + m[1] + ' ' + m[2];
        return raw;
    },

    load: function() {
        return fs.read('/usr/lib/os-release')
            .then(function(content) {
                // OPENWRT_RELEASE="OpenWrt 24.10.7~4g-lte-202606-rev01"
                var m = (content || '').match(/OPENWRT_RELEASE="(OpenWrt\s+[\d.]+)~.*-(\d{6}-rev\d+)"/);
                if (m) return m[1].trim() + ' ' + m[2].trim();
                return '';
            })
            .catch(function() { return ''; });
    },

    checkInitialState: function(currentVer) {
        var self = this;
        // Show current version immediately from fw_rev
        self.updateVersionBlock(currentVer, '—', false);

        // Restore available version if previous check result still on disk
        self.verifyCheckResult(currentVer).then(function(success) {
            if (success) {
                self.upgradeButton.disabled = false;
            }
        }).catch(function() {});
    },

    updateVersionBlock: function(current, upgrade, hasUpdate) {
        this.verCurrentVal.textContent = current || '—';
        this.verAvailVal.textContent   = upgrade  || '—';

        // Changelog link visible only when update is available
        this.verChangelogLink.style.display = hasUpdate ? '' : 'none';

        // Colour: important (green) = update found, warning (yellow) = no update / unknown
        this.versionBlock.className = 'alert-message ' + (hasUpdate ? 'success' : 'warning');
    },

    showChangelog: function() {
        ui.showModal(_('Changelog'), [
            E('pre', {
                'style': 'white-space: pre-wrap; max-height: 60vh; overflow-y: auto; ' +
                         'background: #f5f5f5; padding: 10px; border-radius: 3px; color: black;'
            }, this.changelog || _('(empty)')),
            E('div', { 'class': 'right' }, [
                E('button', {
                    'class': 'btn cbi-button',
                    'click': ui.hideModal
                }, _('Close'))
            ])
        ]);
    },

    render: function(data) {
        var self = this;

        this.checkButton = E('button', {
            'class': 'btn cbi-button cbi-button-positive important',
            'click': ui.createHandlerFn(this, 'handleCheck')
        }, _('Check for Updates'));

        this.upgradeButton = E('button', {
            'class': 'btn cbi-button cbi-button-negative important',
            'disabled': true,
            'click': ui.createHandlerFn(this, 'handleUpgrade')
        }, _('Upgrade'));

        // Version info rows — always visible, updated after check
        this.verCurrentVal  = E('span', {}, '—');
        this.verAvailVal    = E('span', {}, '—');
        this.verChangelogLink = E('button', {
            'class': 'btn cbi-button cbi-button-neutral',
            'style': 'display: none; margin-left: 10px;',
            'click': function() { self.showChangelog(); }
        }, _('Show Changelog'));

        this.versionBlock = E('div', { 'class': 'alert-message', 'style': 'margin-top: 1.5em;' }, [
            E('table', { 'style': 'border: none; background: none;' }, [
                E('tr', {}, [
                    E('td', { 'style': 'padding: 2px 10px 2px 0; font-weight: bold; white-space: nowrap;' },
                        _('Current version:')),
                    E('td', {}, this.verCurrentVal)
                ]),
                E('tr', {}, [
                    E('td', { 'style': 'padding: 2px 10px 2px 0; font-weight: bold; white-space: nowrap;' },
                        _('Available version:')),
                    E('td', {}, [ this.verAvailVal, this.verChangelogLink ])
                ])
            ])
        ]);

        var container = E('div', { 'class': 'cbi-map' }, [
            E('h2', {}, _('OTA System Update')),
            E('div', { 'class': 'cbi-section' }, [
                this.checkButton,
                ' ',
                this.upgradeButton
            ]),
            this.versionBlock
        ]);

        this.checkInitialState(data);

        return container;
    },

    handleCheck: function() {
        var self = this;

        this.checkButton.disabled = true;
        this.upgradeButton.disabled = true;

        return fs.exec('/usr/share/ota.sh', ['check'])
            .then(function() {
                return self.verifyCheckResult(self.verCurrentVal.textContent);
            })
            .then(function(success) {
                self.upgradeButton.disabled = !success;
                self.checkButton.disabled = false;
            })
            .catch(function(err) {
                self.updateVersionBlock('', _('Check failed: ') + (err.message || err), false);
                self.checkButton.disabled = false;
            });
    },

    verifyCheckResult: function(currentVer) {
        var self = this;

        return Promise.all([
            fs.stat('/tmp/profiles.json').catch(function() { return null; }),
            fs.stat('/tmp/update.lock').catch(function() { return null; }),
            fs.read('/tmp/changelog.txt')
                .then(function(content) {
                    self.changelog = content;
                    return content && content.length > 0;
                })
                .catch(function() { return false; }),
            fs.read('/tmp/ota_version')
                .then(function(content) { return content || ''; })
                .catch(function() { return ''; })
        ]).then(function(results) {
            var hasProfiles  = results[0] !== null;
            var hasLock      = results[1] !== null;
            var hasChangelog = results[2];
            var verContent   = results[3];

            // both current= and upgrade= are full firmware names written by ota.sh
            var current = '', upgrade = '';
            verContent.split('\n').forEach(function(line) {
                var m;
                if ((m = line.match(/^current=(.+)/))) current = self.fmtVer(m[1].trim());
                if ((m = line.match(/^upgrade=(.+)/))) upgrade = self.fmtVer(m[1].trim());
            });
            // fallback: if ota_version missing, use value from load() (already formatted)
            if (!current) current = currentVer || '';

            if (hasProfiles && hasLock && hasChangelog) {
                self.updateVersionBlock(current, upgrade, true);
                return true;
            }

            self.updateVersionBlock(current, _('No updates available'), false);
            return false;
        });
    },

    // ----------------------------------------------------------------
    // Upgrade modal
    // ----------------------------------------------------------------

    handleUpgrade: function() {
        var self = this;

        this.upgradeButton.disabled = true;
        this.checkButton.disabled = true;

        self.openUpgradeModal();

        // ota-launch.sh starts ota.sh in background and exits immediately
        return fs.exec('/usr/share/ota-launch.sh', [])
            .then(function() {
                // launcher exited — polling is running, wait for 'done' or 'error'
            })
            .catch(function(err) {
                var msg = err.message || err.toString();
                // XHR timeout is expected — launcher or sysupgrade still running,
                // polling continues and will finalize via 'flashing'/'error' state
                if (msg.indexOf('timed out') !== -1 || msg.indexOf('XHR') !== -1) {
                    return;
                }
                // Genuine exec failure
                self.stopProgressPolling();
                self.setModalProgress(0, _('Launch failed: ') + msg, 'error');
                self.finalizeUpgradeModal(true);
            });
    },

    openUpgradeModal: function() {
        var self = this;

        self.modalProgressInner = E('div', { 'style': 'width: 0%; transition: width 0.4s ease;' });
        self.modalProgressBar = E('div', {
            'class': 'cbi-progressbar',
            'style': 'margin: 10px 0;'
        }, self.modalProgressInner);

        self.modalStatus = E('div', {
            'style': 'font-size: 0.9em; color: #666; margin-bottom: 8px;'
        }, _('Starting upgrade...'));

        self.modalLog = E('pre', {
            'style': 'white-space: pre-wrap; max-height: 200px; overflow-y: auto; ' +
                     'background: #f5f5f5; padding: 8px; border-radius: 3px; ' +
                     'color: black; font-size: 0.85em; margin-top: 10px;'
        }, '');

        self.modalCloseBtn = E('button', {
            'class': 'btn cbi-button',
            'disabled': true,
            'click': function() {
                self.stopProgressPolling();
                ui.hideModal();
                self.upgradeButton.disabled = false;
                self.checkButton.disabled = false;
            }
        }, _('Close'));

        ui.showModal(_('Upgrade Progress'), [
            self.modalStatus,
            self.modalProgressBar,
            self.modalLog,
            E('div', { 'class': 'right', 'style': 'margin-top: 10px;' }, [
                self.modalCloseBtn
            ])
        ]);

        self.startProgressPolling();
    },

    startProgressPolling: function() {
        var self = this;
        if (self.pollTimer) clearInterval(self.pollTimer);
        self.lastLogLine = '';

        self.pollTimer = setInterval(function() {
            fs.read('/tmp/ota_progress').then(function(raw) {
                if (!raw) return;
                var line = raw.trim();
                if (line === self.lastLogLine) return;
                self.lastLogLine = line;

                if (line.indexOf('downloading') === 0) {
                    var pct = parseInt(line.split(' ')[1], 10) || 0;
                    var barPct = Math.round(pct * 0.70);
                    self.setModalProgress(barPct,
                        _('Downloading firmware: ') + pct + '%', 'progress');
                    self.appendModalLog(_('Downloading: ') + pct + '%');

                } else if (line === 'verifying') {
                    self.setModalProgress(75, _('Verifying SHA256 checksum...'), 'progress');
                    self.appendModalLog(_('Verifying SHA256...'));

                } else if (line === 'testing') {
                    self.setModalProgress(85, _('Testing firmware image (sysupgrade -T)...'), 'progress');
                    self.appendModalLog(_('Testing firmware image...'));

                } else if (line === 'flashing') {
                    self.stopProgressPolling();
                    self.setModalProgress(95, _('Flashing! DO NOT POWER OFF!'), 'progress');
                    self.appendModalLog(_('Flashing firmware...'));
                    // Simulate reboot wait with countdown
                    var countdown = 90;
                    self.setModalProgress(98, _('Device will be rebooted, please wait... ') + countdown + 's', 'progress');
                    var rebootTimer = setInterval(function() {
                        countdown--;
                        if (countdown <= 0) {
                            clearInterval(rebootTimer);
                            self.setModalProgress(100, _('Device will be rebooted, please wait... '), 'done');
                            self.finalizeUpgradeModal(false);
                        } else {
                            self.setModalProgress(98, _('Device will be rebooted, please wait... ') + countdown + 's', 'progress');
                        }
                    }, 1000);

                } else if (line === 'done') {
                    self.stopProgressPolling();
                    self.setModalProgress(100, _('Upgrade started. Device will reboot...'), 'done');
                    self.appendModalLog(_('Done. Waiting for reboot...'));
                    self.finalizeUpgradeModal(false);

                } else if (line.indexOf('error') === 0) {
                    var reason = line.replace('error', '').trim();
                    self.stopProgressPolling();
                    self.setModalProgress(0, _('Error: ') + reason, 'error');
                    self.appendModalLog(_('FAILED: ') + reason);
                    self.finalizeUpgradeModal(true);
                }
            }).catch(function() {});
        }, 1500);
    },

    stopProgressPolling: function() {
        if (this.pollTimer) {
            clearInterval(this.pollTimer);
            this.pollTimer = null;
        }
    },

    setModalProgress: function(percent, text, state) {
        if (!this.modalProgressInner || !this.modalStatus) return;
        var pct = Math.min(100, Math.max(0, percent));
        this.modalProgressInner.style.width = pct + '%';
        this.modalStatus.textContent = text;
        if (state === 'error') {
            this.modalStatus.style.color = '#c00';
        } else if (state === 'done') {
            this.modalStatus.style.color = '#080';
        } else {
            this.modalStatus.style.color = '#666';
        }
    },

    appendModalLog: function(line) {
        if (!this.modalLog) return;
        this.modalLog.textContent += line + '\n';
        this.modalLog.scrollTop = this.modalLog.scrollHeight;
    },

    finalizeUpgradeModal: function(isError) {
        if (this.modalCloseBtn) {
            this.modalCloseBtn.disabled = false;
        }
        if (!isError) {
            this.versionBlock.className = 'alert-message warning';
            this.verAvailVal.textContent = _('Upgrade started — device will reboot');
            this.verChangelogLink.style.display = 'none';
        } else {
            this.upgradeButton.disabled = false;
            this.checkButton.disabled = false;
        }
    },

    handleSaveApply: null,
    handleSave: null,
    handleReset: null
});
