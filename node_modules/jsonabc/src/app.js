/*!
  Form / application "onsubmit" handler, and analytics.
*/

var jsonabc = require('jsonabc');

window.appSort = appSort;

function appSort (ev, tid) {
  var inputStr = document.getElementById(tid).value;
  var noarray = document.getElementById('noarray').checked;

  ev.preventDefault();

  try {
    var output = jsonabc.sort(inputStr, noarray);

    document.getElementById(tid).value = output;

    console.warn('jsonabc input:', JSON.parse(inputStr), noarray);
  } catch (ex) {
    window.alert('Incorrect JSON object');
  }
}

/* eslint-disable */
(function (i, s, o, g, r, a, m) {
    i['GoogleAnalyticsObject'] = r;
    i[r] = i[r] || function () {
        (i[r].q = i[r].q || []).push(arguments)
    }, i[r].l = 1 * new Date();
    a = s.createElement(o),
    m = s.getElementsByTagName(o)[0];
    a.async = 1;
    a.src = g;
    m.parentNode.insertBefore(a, m)
})(window, document, 'script', 'https://www.google-analytics.com/analytics.js', 'ga');

ga('create', 'UA-58536835-1', 'auto');
ga('send', 'pageview');
/* eslint-enable */
