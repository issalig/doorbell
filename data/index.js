//https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/ for Ajax
//https://circuits4you.com/2019/01/11/esp8266-data-logging-with-real-time-graphs/

var rings=[];

function processData(data)
{
  let json = JSON.parse(data); 
  console.log(json);

  if (json.id=="rings") parseRings(json);
}

//Ajax script to get ADC voltage at every 5 Seconds 
 
setInterval(function() {
  // Call a function repetatively with 5 Second interval
  getData();
}, 5000); //5000mSeconds update rate

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      //Push the data in array
      console.log(this.responseText);
	  var json = JSON.parse(this.responseText); 
	  parseData(json);
    }
  };
  xhttp.open("GET", "getRings", true);	//Handle getRings server on ESP8266
  xhttp.send();
}

function parseData(json){		
	var table = document.getElementById("dataTable");
    var d = new Date(parseInt(json.time)*1000);
    
    //var dec = moment(parseInt(json.time)*1000);
    //dec.tz('Europe/Brussels').format('ha z');
 
	document.getElementById("doorbell_time").innerHTML= json.time*1000 +"<br>"+ d.toLocaleTimeString() + " " + d.toLocaleDateString()


    //clear data if any
    for(var i = table.rows.length - 1; i > 0; i--)
    {
        table.deleteRow(i);
    }
	
	//add new data
    for(i=0;i<json.vals.length;i++){		 	
	  var row = table.insertRow(-1);	//Add after headings
	  var cell1 = row.insertCell(0);
	  if (json.vals[i] != "-"){
		d = new Date(parseInt(json.vals[i]*1000));
		cell1.innerHTML = d.toLocaleTimeString() + " " + d.toLocaleDateString();	
	  }
	  else
		cell1.innerHTML = json.vals[i];
		
	  //var time = new Date().toLocaleTimeString();
    } 

}

getData();
//On Page load show graphs
//window.onload = function() {
//};

