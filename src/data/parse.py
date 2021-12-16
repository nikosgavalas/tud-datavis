import csv
import json

mappings = {
    'SP.POP.TOTL': 'population-total',
    'SP.POP.1564.TO.ZS': 'population-working-age',
    'SP.POP.GROW': 'population-growth',
    'SP.DYN.LE00.IN': 'life-expectancy',
    'NY.GDP.PCAP.CD': 'gdp-per-capita',
    'NY.GDP.MKTP.CD': 'gdp',
}

data = {}
with open('./data.csv', 'r') as inf:
    reader = csv.reader(inf, delimiter=',', quotechar='"')
    next(reader)  # skip header
    for row in reader:
        if row[1] not in data:
            data[row[1]] = {}
        if row[3] in mappings:
            data[row[1]][mappings[row[3]]] = row[4:]


with open('./data.json', 'w') as outf:
    outf.write(json.dumps(data))
