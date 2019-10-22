var fs = require('fs')


var hrstart = process.hrtime()
//11000000
var s = 0;
var n = 11000000
for ( var i = 0; i < n; i++ ) {
	s = s + i
}



const NS_PER_SEC = 1e9;
var hrend = process.hrtime(hrstart)
console.log(`etime: ${(hrend[0] * NS_PER_SEC + hrend[1])/NS_PER_SEC}`);

const used = process.memoryUsage();
for (let key in used) {
  console.log(`${key} ${Math.round(used[key] / 1024 / 1024 * 100) / 100} MB`);
}
