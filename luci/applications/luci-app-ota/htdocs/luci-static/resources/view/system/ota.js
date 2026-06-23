'use strict';
'require dom';
'require fs';
'require ui';
'require view';

/*
 * OTA Update Application
 */

return view.extend({
    load: function() {
        return Promise.resolve();
    },

    checkInitialState: function() {
        var self = this;
        this.verifyCheckResult().then(function(success) {
            if (success) {
                self.checkButton.disabled = true;
                self.showUpdateAvailable();
                self.upgradeButton.disabled = false;
            }
        }).catch(function() {});
    },

    showUpdateAvailable: function() {
        var self = this;
        self.statusDiv.innerHTML = '';
        self.statusDiv.appendChild(E('div', { 'class': 'alert-message success' }, [
            E('h3', {}, _('Update Available')),
            E('button', {
                'class': 'btn cbi-button cbi-button-neutral',
                'style': 'margin-top: 6px;',
                'click': function() { self.showChangelog(); }
            }, _('Show Changelog'))
        ]));
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

    render: function() {
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

        this.statusDiv = E('div', { 'class': 'cbi-section' });

        var container = E('div', { 'class': 'cbi-map' }, [
            E('h2', {}, _('OTA System Update')),
            E('div', { 'class': 'cbi-section' }, [
                this.checkButton,
                ' ',
                this.upgradeButton
            ]),
            this.statusDiv
        ]);

        this.checkInitialState();

        return container;
    },

    handleCheck: function() {
        var self = this;

        this.checkButton.disabled = true;
        this.statusDiv.innerHTML = '';
        this.statusDiv.appendChild(E('div', { 'class': 'spinner' }, _('Checking for updates...')));

        return fs.exec('/usr/share/ota.sh', ['check'])
            .then(function() {
                return self.verifyCheckResult();
            })
            .then(function(success) {
                if (success) {
                    self.upgradeButton.disabled = false;
                    self.showUpdateAvailable();
                } else {
                    self.statusDiv.innerHTML = '';
                    self.statusDiv.appendChild(E('div', { 'class': 'alert-message warning' },
                        _('No updates available or check failed')));
                }
                self.checkButton.disabled = false;
            })
            .catch(function(err) {
                self.statusDiv.innerHTML = '';
                self.statusDiv.appendChild(E('div', { 'class': 'alert-message error' },
                    _('Check failed: ') + (err.message || err)));
                self.checkButton.disabled = false;
            });
    },

    verifyCheckResult: function() {
        var self = this;

        return Promise.all([
            fs.stat('/tmp/profiles.json').catch(function() { return null; }),
            fs.stat('/tmp/update.lock').catch(function() { return null; }),
            fs.read('/tmp/changelog.txt')
                .then(function(content) {
                    self.changelog = content;
                    return content && content.length > 0;
                })
                .catch(function() { return false; })
        ]).then(function(results) {
            return results[0] !== null &&
                   results[1] !== null &&
                   results[2] === true;
        });
    },

    // ----------------------------------------------------------------
    // Upgrade modal
    // ----------------------------------------------------------------

    handleUpgrade: function() {
        var self = this;

        this.upgradeButton.disabled = true;
        this.checkButton.disabled = true;

        // Remove stale progress file before starting
        fs.remove('/tmp/ota_progress').catch(function() {});

        self.openUpgradeModal();

        return fs.exec('/usr/share/ota.sh', ['upgrade'])
            .then(function() {
                self.stopProgressPolling();
                self.setModalProgress(100, _('Upgrade started. Device will reboot...'), 'done');
                self.finalizeUpgradeModal(false);
            })
            .catch(function(err) {
                var msg = err.message || err.toString();
                self.stopProgressPolling();

                // XHR timeout = sysupgrade already running, treat as success
                if (msg.indexOf('timed out') !== -1) {
                    self.setModalProgress(100, _('Upgrade started. Device will reboot...'), 'done');
                    self.finalizeUpgradeModal(false);
                } else {
                    self.setModalProgress(0, _('Error: ') + msg, 'error');
                    self.finalizeUpgradeModal(true);
                }
            });
    },

    openUpgradeModal: function() {
        var self = this;

        // Progress bar element
        self.modalProgressInner = E('div', { 'style': 'width: 0%; transition: width 0.4s ease;' });
        self.modalProgressBar = E('div', { 'class': 'cbi-progressbar', 'style': 'margin: 10px 0;' },
            self.modalProgressInner);

        // Status label
        self.modalStatus = E('div', {
            'style': 'font-size: 0.9em; color: #666; margin-bottom: 8px;'
        }, _('Starting upgrade...'));

        // Log area
        self.modalLog = E('pre', {
            'style': 'white-space: pre-wrap; max-height: 200px; overflow-y: auto; ' +
                     'background: #f5f5f5; padding: 8px; border-radius: 3px; ' +
                     'color: black; font-size: 0.85em; margin-top: 10px;'
        }, '');

        // Close button (disabled until done)
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

        // Start polling /tmp/ota_progress
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
                    // downloading maps to 0..70% of bar
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
                    self.setModalProgress(95, _('Flashing! DO NOT POWER OFF!'), 'progress');
                    self.appendModalLog(_('Flashing firmware...'));

                } else if (line.indexOf('error') === 0) {
                    var reason = line.replace('error', '').trim();
                    self.setModalProgress(0, _('Error: ') + reason, 'error');
                    self.appendModalLog(_('FAILED: ') + reason);
                    self.stopProgressPolling();
                }
            }).catch(function() {
                // file not yet created — ignore
            });
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
        // Auto-scroll to bottom
        this.modalLog.scrollTop = this.modalLog.scrollHeight;
    },

    finalizeUpgradeModal: function(isError) {
        if (this.modalCloseBtn) {
            this.modalCloseBtn.disabled = false;
        }
        if (!isError) {
            // For successful flash — don't re-enable buttons,
            // device is about to reboot
            this.statusDiv.innerHTML = '';
            this.statusDiv.appendChild(E('div', { 'class': 'alert-message warning' },
                _('Upgrade started! DO NOT POWER OFF THIS DEVICE! System will reboot after upgrade.')));
        } else {
            this.upgradeButton.disabled = false;
            this.checkButton.disabled = false;
        }
    },

    handleSaveApply: null,
    handleSave: null,
    handleReset: null
});
