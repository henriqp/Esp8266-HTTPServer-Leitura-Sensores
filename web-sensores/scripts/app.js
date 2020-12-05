function getTempColor(t) {
  if (t >= 35) {
    return '#ff5722';
  } else if (t >= 30) {
    return '#ff9800';
  } else if (t >= 25) {
    return '#ffc107';
  } else if (t >= 18) {
    return '#4caf50';
  } else if (t > 10) {
    return '#8bc34a';
  } else if (t >= 5) {
    return '#00bcd4';
  } else if (t >= -5) {
    return '#03a9f4';
  } else {
    return '#2196f3';
  }
}

function getHumColor(x) {
  var colors = ['#E3F2FD','#BBDEFB','#90CAF9','#64B5F6','#42A5F5','#2196F3','#1E88E5','#1976D2','#1565C0','#0D47A1','#0D47A1'];

  return colors[Math.round(x / 10)];
}

function getTemperatura() {
    var xhr = new XMLHttpRequest();

    xhr.onload = function(data)
    {
        if (data.currentTarget.status == 200) {
            var temperatura = parseInt(data.currentTarget.responseText);

            tempGauge.setVal(temperatura).setColor(getTempColor(temperatura));
        }
    }

    xhr.open('GET', 'http://192.168.0.104/temperatura', true);
    xhr.send();
}

function getUmidade() {
    var xhr = new XMLHttpRequest();

    xhr.onload = function(data)
    {
        if (data.currentTarget.status == 200) {
            var umidade = parseInt(data.currentTarget.responseText);

            humGauge.setVal(umidade).setColor(getHumColor(umidade));
        }
    }

    xhr.open('GET', 'http://192.168.0.104/umidade', true);
    xhr.send();
}

function getDistancia() {
    var xhr = new XMLHttpRequest();

    xhr.onload = function(data)
    {
        if (data.currentTarget.status == 200) {
            var $span = document.getElementById('dist'),
                distancia = parseInt(data.currentTarget.responseText);

            $span.innerHTML = distancia + 'cm';
        }
    }

    xhr.open('GET', 'http://192.168.0.104/distancia', true);
    xhr.send();
}

var tempGauge = createVerGauge('temp', -20, 60, ' °C').setVal(0).setColor(getTempColor(0));
var humGauge = createRadGauge('hum', 0, 100, '%').setVal(0).setColor(getHumColor(0));

function getInformacoesSensores() {
    getTemperatura();
    getUmidade();
    getDistancia();
}

window.onload = getInformacoesSensores;

setInterval(getInformacoesSensores, 5000);