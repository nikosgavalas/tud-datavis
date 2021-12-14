const map = {};

map.svg = d3.select("#mapArea").append("svg");
map.aspectRatio = 1.3;
map.width = document.getElementById("mapArea").clientWidth;
map.height = map.width / map.aspectRatio;

map.svg.attr("width", map.width).attr("height", map.height);

function resize() {
  map.width = document.getElementById("mapArea").clientWidth;
  map.height = map.width / map.aspectRatio;
  map.svg.attr("width", map.width).attr("height", map.height);
}

window.onresize = resize;

const projection = d3
  .geoNaturalEarth1()
  .scale(map.width / 1.5 / Math.PI)
  .translate([map.width / 2.3, map.height / 2.3]);

const path = d3.geoPath().projection(projection);

const color = d3.scaleLinear([50, 75], ["blue", "white"]);

var renderedMap = map.svg
  .append("g")
  .selectAll("path")
  .data(geojson.features)
  .join("path")
  .attr("d", path)
  .attr("fill", (d, i) => {
    var value = d.id in data ? data[d.id]["population-working-age"][0] : 0;
    if (value == "..") {
      value = 0;
    }
    return color(value);
  });
//.style("stroke", "#fff");

function changeSize(size) {
  renderedMap.attr("fill", (d, i) => {
    var idx = 2020 - size;
    var value = d.id in data ? data[d.id]["population-working-age"][idx] : 0;
    if (value == "..") {
      value = 0;
    }
    return color(value);
  });
}

d3.select("#timeSlider").on("input", function (d) {
  selectedValue = this.value;
  changeSize(selectedValue);
});
