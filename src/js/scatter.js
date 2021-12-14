// const svg = d3.select("#scatter"),
//   width = +svg.attr("width"),
//   height = +svg.attr("height");

// var coords = [0, 100, 200, 300, 400];
var svg = d3.select("#test");

var circle = svg.selectAll("circle")
    .data([32, 57, 112, 293]);

var circleEnter = circle.enter().append("circle");

circleEnter.attr("cy", 60);
circleEnter.attr("cx", function(d, i) { return i * 100 + 30; });
circleEnter.attr("r", function(d) { return Math.sqrt(d); });

// d3.json("./data/scatter-data.json").then(function (data) {
//   const circle = svg.append("g")
//   .attr("stroke", "black")
//   .selectAll("circle")
//   .data(data)
//   .join("circle")
//   // .sort((a, b) => d3.descending(a.population, b.population))
//   .attr("cx", d => x(d.income))
//   .attr("cy", d => y(d.lifeExpectancy))
//   .attr("r", d => radius(d.population))
//   .attr("fill", d => color(d.region))
//   .call(circle => circle.append("title")
//   .text(d => [d.name, d.region].join("\n")));

// });
