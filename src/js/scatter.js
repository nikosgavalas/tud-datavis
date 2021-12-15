const scatter = {};

scatter.margin = { top: 10, right: 30, bottom: 30, left: 30 };
scatter.aspectRatio = 1.3;
scatter.width =
  document.getElementById("scatterArea").clientWidth -
  scatter.margin.left -
  scatter.margin.right;
scatter.height =
  scatter.width / scatter.aspectRatio -
  scatter.margin.top -
  scatter.margin.bottom;

scatter.svg = d3
  .select("#scatterArea")
  .append("svg")
  .attr("width", scatter.width + scatter.margin.left + scatter.margin.right)
  .attr("height", scatter.height + scatter.margin.top + scatter.margin.bottom)
  .append("g")
  .attr(
    "transform",
    "translate(" + scatter.margin.left + "," + scatter.margin.top + ")"
  );

scatter.svg.attr("width", scatter.width).attr("height", scatter.height);

// function resize() {
//   scatter.width = document.getElementById("scatterArea").clientWidth;
//   scatter.height = scatter.width / scatter.aspectRatio;
//   scatter.svg.attr("width", scatter.width).attr("height", scatter.height);
// }

// window.onresize = resize;

// gdp
var x = d3.scaleLog([1, 100000000], [0, scatter.width]);
scatter.svg
  .append("g")
  .attr("transform", "translate(0," + scatter.height + ")")
  .call(d3.axisBottom(x).tickSize(-scatter.height).ticks());

// population growth
var y = d3.scaleLinear([-5, 5], [scatter.height, 0]);
scatter.svg.append("g").call(d3.axisLeft(y).tickSize(-scatter.width).ticks());

scatter.svg.selectAll(".tick line").attr("stroke", "lightgray");

scatter.svg
  .append("g")
  .selectAll("circle")
  .data(Object.keys(data))
  .enter()
  .append("circle")
  .attr("cx", (d, i) => {
    return x(parseFloat(data[d]["gdp"][0]) / 1000000);
  })
  .attr("cy", (d, i) => {
    return y(parseFloat(data[d]["population-growth"][0]));
  })
  .attr("r", (d, i) => {
    var x = data[d]["gdp"][0];
    var y = data[d]["population-growth"][0];
    var r = data[d]["population"][0];
    if (x == ".." || y == "..") {
      return 0;
    }
    return 10;
  })
  .style("fill", "#69b3a2");
