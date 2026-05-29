'use strict';
'require ui';
'require poll';
'require fs';
'require uci';

function getSignalColor(percent) {
    if (percent === null || percent === undefined || percent < 5) {
        return '#000000';
    }
    
    let p = percent;
    
    if (p >= 80) return '#00cc00';
    
    if (p >= 67) {
        const ratio = (p - 50) / 30;
        const r = Math.floor(255 * (1 - ratio));
        const g = 255;
        return `rgb(${r}, ${g}, 0)`;
    }
    
    if (p >= 35) {
        const ratio = (p - 25) / 25;
        const r = 255;
        const g = Math.floor(165 + (90 * ratio));
        return `rgb(${r}, ${g}, 0)`;
    }
    
    const ratio = (p - 5) / 20;
    const r = 255;
    const g = Math.floor(165 * ratio);
    return `rgb(${r}, ${g}, 0)`;
}


function getSignalState(percent) {
    const p = percent || 0;
    if (p < 10) return 'error';
    if (p < 25) return 'warning';
    if (p < 50) return 'warning';
    return 'active';
}

function isUciIndexTwo() {
    return L.resolveDefault(fs.exec_direct('/sbin/uci', ['get', 'modeminfo.@general[0].index']), '')
        .then(function(value) {
            return value.trim() === '2';
        })
        .catch(function() {
            return false;
        });
}

function updateIndicator() {



    isUciIndexTwo().then(function(ok) {
        if (!ok) {
            ui.hideIndicator('modem-status');
            return;
        }

    if (window.location.pathname.includes('/cgi-bin/luci/admin/modem/main')) {
        ui.hideIndicator('modem-status');
        return;
    }
    L.resolveDefault(fs.exec_direct('/usr/bin/modeminfo'), '{"modem":[]}')
    .then(function(res) {
        var data = JSON.parse(res);
        if (!data.modem || !data.modem.length) {
            ui.hideIndicator('modem-status');
            return;
        }

        var parts = data.modem.map(function(modem) {
            var percent = parseInt(modem.csq_per);
            var cops = modem.cops || '';
            //var mode = modem.mode + (parseInt(modem.lteca) > 0 ? '+' : '');
	    var mode = modem.mode;
	    if (modem.mode !== 'LTE+NR' && parseInt(modem.lteca) > 0) mode += '+';
            return { mode: mode, cops: cops, percent: percent };
        });

        var minPercent = Math.min.apply(null, parts.map(function(p) { return p.percent; }));
        var state = getSignalState(minPercent);

        var status = parts.map(function(p) {
            return p.mode + ' ' + p.cops;
        }).join(' | ');

        ui.showIndicator('modem-status', status, null, state);

        var indicator = document.querySelector('[data-indicator="modem-status"]');
        if (indicator) {
            var html = parts.map(function(p) {
                var color = getSignalColor(p.percent);
		return '<span style="color:' + color + '; -webkit-text-stroke: 0.5px black; text-stroke: 0.5px black;">●</span> ' + p.mode + ' ' + p.cops;
            }).join(' | ');
            indicator.innerHTML = html;
        }
    })
    .catch(function() {
        ui.hideIndicator('modem-status');
    });
    });
}

setTimeout(function() {
    updateIndicator();
    poll.add(updateIndicator, 10);
}, 500);

return L.Class.extend({
    __name__: 'ModemIndicator'
});
