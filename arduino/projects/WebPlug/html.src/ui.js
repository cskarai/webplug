//===== AJAX

/*
 * Get cross browser xhr object
 *
 * Copyright (C) 2011 Jed Schmidt <http://jed.is>
 * More: https://gist.github.com/993585
 */

var j = function(
  a // cursor placeholder
){
  for(                     // for all a
    a=0;                   // from 0
    a<4;                   // to 4,
    a++                    // incrementing
  ) try {                  // try
    return a               // returning
      ? new ActiveXObject( // a new ActiveXObject
          [                // reflecting
            ,              // (elided)
            "Msxml2",      // the various
            "Msxml3",      // working
            "Microsoft"    // options
          ][a] +           // for Microsoft implementations, and
          ".XMLHTTP"       // the appropriate suffix,
        )                  // but make sure to
      : new XMLHttpRequest // try the w3c standard first, and
  }

  catch(e){}               // ignore when it fails.
}

function ajaxReq(method, url, ok_cb, err_cb, data) {
  var xhr = j();
  xhr.open(method, url, true);
  var timeout = setTimeout(function() {
    xhr.abort();
    console.log("XHR abort:", method, url);
    xhr.status = 599;
    xhr.responseText = "request time-out";
  }, 9000);
  xhr.onreadystatechange = function() {
    if (xhr.readyState != 4) { return; }
    clearTimeout(timeout);
    if (xhr.status >= 200 && xhr.status < 300) {
//      console.log("XHR done:", method, url, "->", xhr.status);
      ok_cb(xhr.responseText);
    } else {
      console.log("XHR ERR :", method, url, "->", xhr.status, xhr.responseText, xhr);
      err_cb(xhr.status, xhr.responseText);
    }
  }
//  console.log("XHR send:", method, url);
  try {
    xhr.send(data);
  } catch(err) {
    console.log("XHR EXC :", method, url, "->", err);
    err_cb(599, err);
  }
}

function dispatchJson(resp, ok_cb, err_cb) {
  var j;
  try { j = JSON.parse(resp); }
  catch(err) {
    console.log("JSON parse error: " + err + ". In: " + resp);
    err_cb(500, "JSON parse error: " + err);
    return;
  }
  ok_cb(j);
}

function ajaxJson(method, url, ok_cb, err_cb) {
  ajaxReq(method, url, function(resp) { dispatchJson(resp, ok_cb, err_cb); }, err_cb);
}

//===== chain onload handlers

function onLoad(f) {
  var old = window.onload;
  if (typeof old != 'function') {
    window.onload = f;
  } else {
    window.onload = function() {
      old();
      f();
    }
  }
}

//===== PAGE LOAD

function loadParams(name, reloadttime) {
  // populate menu via ajax call
  var loadFunction = function() {
    ajaxJson("GET", name, function(data) {
      for (var i = 0; i < data.list.length; i++) {
        if( data.list[i][2] == 'value' )
          document.getElementById(data.list[i][0]).value=data.list[i][1];
        else if( data.list[i][2] == 'innerHTML' )
          document.getElementById(data.list[i][0]).innerHTML=data.list[i][1];
      }
      
      if( reloadttime )
      {
        setTimeout(loadFunction, 1000);
      }
    }, function() { setTimeout(loadFunction, 1000); });
  };
  loadFunction();
}

//===== Menu button handling

function addClass(el, cl) {
  el.className += ' ' + cl;
}

function removeClass(el, cl) {
  var cls = el.className.split(/\s+/),
      l = cls.length;
  for (var i=0; i<l; i++) {
    if (cls[i] === cl) cls.splice(i, 1);
  }
  el.className = cls.join(' ');
  return cls.length != l
}

function toggleClass(el, cl) {
  if (!removeClass(el, cl)) addClass(el, cl);
}

function initMenu()
{
  var el = document.getElementById("menuLink");
  el.addEventListener('click', function(e) {
    e.preventDefault();
        
    var active = 'active';
    toggleClass(document.getElementById("layout"), active);
    toggleClass(document.getElementById("menu"), active);
    toggleClass(document.getElementById("menuLink"), active);

  }, false);
}

//===== Button handling

function registerButton(btnId)
{
  var el = document.getElementById(btnId);

  el.addEventListener('click', function(e) {
    e.preventDefault();
    ajaxReq("POST", window.location.pathname + ".button?id=" + btnId, function(e) {}, function (e) {});
  }, false);
}

function loadScript(url)
{    
  var head = document.getElementsByTagName('head')[0];
  var script = document.createElement('script');
  script.type = 'text/javascript';
  script.src = url;
  head.appendChild(script);
}
