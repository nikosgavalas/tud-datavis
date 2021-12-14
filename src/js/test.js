///////////////----Scatter Plot---- ///////////////

//Creation of grid
grid = (g) =>
  g
    .attr("stroke", "currentColor")
    .attr("stroke-opacity", 0.1)
    .call((g) =>
      g
        .append("g")
        .selectAll("line")
        .data(x.ticks())
        .join("line")
        .attr("x1", (d) => 0.5 + x(d))
        .attr("x2", (d) => 0.5 + x(d))
        .attr("y1", margin.top)
        .attr("y2", height - margin.bottom)
    )
    .call((g) =>
      g
        .append("g")
        .selectAll("line")
        .data(y.ticks())
        .join("line")
        .attr("y1", (d) => 0.5 + y(d))
        .attr("y2", (d) => 0.5 + y(d))
        .attr("x1", margin.left)
        .attr("x2", width - margin.right)
    );

//Y axis
yAxis = (g) =>
  g
    .attr("transform", `translate(${margin.left},0)`)
    .call(d3.axisLeft(y))
    .call((g) => g.select(".domain").remove())
    .call((g) =>
      g
        .append("text")
        .attr("x", -margin.left)
        .attr("y", 10)
        .attr("fill", "currentColor")
        .attr("text-anchor", "start")
        .text("↑ Life expectancy (years)")
    );

//X axis
xAxis = (g) =>
  g
    .attr("transform", `translate(0,${height - margin.bottom})`)
    .call(d3.axisBottom(x).ticks(width / 80, ","))
    .call((g) => g.select(".domain").remove())
    .call((g) =>
      g
        .append("text")
        .attr("x", width)
        .attr("y", margin.bottom - 4)
        .attr("fill", "currentColor")
        .attr("text-anchor", "end")
        .text("Income per capita (dollars) →")
    );

//New svg

var svg2 = d3.select("#scatter");

// var circle = svg2.selectAll("circle")
//     .data([32, 57, 112, 293]);

// var circleEnter = circle.enter().append("circle");
// circleEnter.attr("cy", 60);
// circleEnter.attr("cx", function(d, i) { return i * 100 + 30; });
// circleEnter.attr("r", function(d) { return Math.sqrt(d); });

d3.json("./data/scatter-data.json").then((data) => {
  console.log(data);

  var circle = svg2
    .selectAll("circle")
    .data(data)
    .join("circle")
    .attr("cx", (d) => d["Fertility rate, total (births per woman)"])
    .attr("cy", (d) => d["GDP (current US$)"])
    .attr("r", (d) => d["Population ages 15-64 (% of total population)"]);

  // var circleEnter = circle.enter().append("circle");

  // circleEnter.attr("cy", 60);
  // circleEnter.attr("cx", function(d, i) { return i * 100 + 30; });
  // circleEnter.attr("r", function(d) { return Math.sqrt(d); });
});
