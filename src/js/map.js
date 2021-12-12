const svg = d3.select("#map"),
  width = +svg.attr("width"),
  height = +svg.attr("height");

const projection = d3
  .geoNaturalEarth1()
  .scale(width / 1.3 / Math.PI)
  .translate([width / 2, height / 2]);
const path = d3.geoPath().projection(projection);

const color = d3.scaleLinear().domain([50, 75]).range(["blue", "white"]);

var countries;
d3.json("./data/map-data.json").then((data) => {
  countries = data;

  d3.json("./data/world.geojson").then(function (data) {
    var map = svg
      .append("g")
      .selectAll("path")
      .data(data.features)
      .join("path")
      .attr("d", path)
      .attr("fill", (d, i) => {
        var value = d.id in countries ? countries[d.id][0] : 0;
        if (value == "..") {
          value = 0;
        }
        return color(value);
      })
      .style("stroke", "#fff");

    function changeSize(size) {
      map.attr("fill", (d, i) => {
        var idx = 2020 - size;
        var value = d.id in countries ? countries[d.id][idx] : 0;
        if (value == "..") {
          value = 0;
        }
        return color(value);
      });
    }

    d3.select("#timeSlider").on("change", function (d) {
      selectedValue = this.value;
      changeSize(selectedValue);
    });
  });
});
