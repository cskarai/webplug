// interpolator script file

var factor = 0.47;
var drawControls = false;
var colorControls = 'orange';

function add(p1, p2) {
  return [p2[0] + p1[0], p2[1] + p1[1]];
}

function subtract(p1, p2) {
  return [p2[0] - p1[0], p2[1] - p1[1]];
}

function multiply(p1, c) {
  return [c*p1[0], c*p1[1]];
}
      
function normalize(p1) {
  var len = Math.sqrt(p1[0] * p1[0] + p1[1] * p1[1]);
  if( len == 0 ) {
    return p1;
  }
  return [p1[0]/len, p1[1]/len];
}

function computeControlVector2(p1, p2) {
  var xmul = Math.abs(p2[0] - p1[0]) * factor;
  return multiply(normalize(subtract(p2, p1)), xmul);
}
      
function computeControlVector3(p1, p2, p3) {
  var d1 = normalize(subtract(p2, p1));
  var d2 = normalize(subtract(p3, p2));
  var added = normalize(add(d1, d2));
  var xmul = Math.abs(p2[0] - p1[0]) * factor;
  var norm = multiply(added, xmul);
  return norm;
}

function computeControlPoints(points, i) {
  var q1;
  var q2;
        
  if( i == 0 ) {
    q1 = add(computeControlVector2(points[1], points[0]), points[0]);
  } else {
    q1 = add(multiply(computeControlVector3(points[i-1], points[i], points[i+1]), -1), points[i]);
  }
        
  if( i < points.length - 2 ) {
    q2 = add(computeControlVector3(points[i], points[i+1], points[i+2]), points[i+1]);
  } else {
    q2 = add(computeControlVector2(points[i], points[i+1]), points[i+1]);
  }
        
  return [q1, q2];
}

function drawCurve(ctx, points) {
  var lwdth = ctx.lineWidth;
  var colorCurve = ctx.strokeStyle;
    
  for (var i = 0; i < points.length; i++) {
    var p = points[i];
          
    ctx.beginPath();
    ctx.arc(p[0], p[1], 2*lwdth, 0, 2 * Math.PI, false);
    ctx.fillStyle = colorCurve;
    ctx.fill();
  }

  for (var i = 0; i < points.length; i++) {
    var p = points[i];
          
    if( i < points.length - 1 ) {
      var controlPoints = computeControlPoints(points, i);
            
      ctx.beginPath();
      ctx.lineWidth = lwdth;
      ctx.moveTo(p[0], p[1]);
      ctx.strokeStyle = colorCurve;
      ctx.bezierCurveTo(controlPoints[0][0],controlPoints[0][1],
                        controlPoints[1][0],controlPoints[1][1],
                        points[i+1][0],points[i+1][1]);
      ctx.stroke();
            
      if( drawControls ) // draw control points
      {
        ctx.beginPath();
        ctx.arc(controlPoints[0][0], controlPoints[0][1], 2*lwdth, 0, 2 * Math.PI, false);
        ctx.fillStyle = colorControls;
        ctx.fill();
        ctx.beginPath();
        ctx.arc(controlPoints[1][0], controlPoints[1][1], 2*lwdth, 0, 2 * Math.PI, false);
        ctx.fillStyle = colorControls;
        ctx.fill();
              
        ctx.beginPath();
        ctx.moveTo(points[i][0], points[i][1]);
        ctx.strokeStyle = colorControls;
        ctx.lineWidth = lwdth / 3;
        ctx.lineTo(controlPoints[0][0], controlPoints[0][1]);
        ctx.moveTo(points[i+1][0], points[i+1][1]);
        ctx.lineTo(controlPoints[1][0], controlPoints[1][1]);
        ctx.stroke();
      }
    }
  }
}
