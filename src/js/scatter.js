const svg = d3.select("#scatter"),
  width = +svg.attr("width"),
  height = +svg.attr("height");


const color = d3.scaleLinear().domain([50, 75]).range(["blue", "white"]);


const circle = svg.append("g")
.attr("stroke", "black")
.selectAll("circle")
.data(dataAt(1800), d => d.name)
.join("circle")
.sort((a, b) => d3.descending(a.population, b.population))
.attr("cx", d => x(d.income))
.attr("cy", d => y(d.lifeExpectancy))
.attr("r", d => radius(d.population))
.attr("fill", d => color(d.region))
.call(circle => circle.append("title")
  .text(d => [d.name, d.region].join("\n")));