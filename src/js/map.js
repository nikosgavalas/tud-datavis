const svg = d3.select("svg"),
  width = +svg.attr("width"),
  height = +svg.attr("height");

const projection = d3
  .geoNaturalEarth1()
  .scale(width / 1.3 / Math.PI)
  .translate([width / 2, height / 2]);
const path = d3.geoPath().projection(projection);

d3.json("./json/world.geojson").then(function (data) {
  svg
    .append("g")
    .selectAll("path")
    .data(data.features)
    .join("path")
    .attr("fill", "#69b3a2")
    .attr("d", path)
    .style("stroke", "#fff");
});
