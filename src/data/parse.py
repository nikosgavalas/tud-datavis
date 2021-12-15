import csv
import json
from types import resolve_bases

data = {}
with open('./data.csv', 'r') as inf:
    reader = csv.reader(inf, delimiter=',', quotechar='"')
    next(reader)  # skip header
    for row in reader:
        if row[1] not in data:
            data[row[1]] = {}
        if row[3] == 'SP.POP.0014.TO.ZS':
            data[row[1]]['population-young'] = row[4:]
        if row[3] == 'SP.POP.1564.TO.ZS':
            data[row[1]]['population-working-age'] = row[4:]
        if row[3] == 'SP.POP.65UP.TO.ZS':
            data[row[1]]['population-old'] = row[4:]
        if row[3] == 'SP.POP.GROW':
            data[row[1]]['population-growth'] = row[4:]
        if row[3] == 'SP.POP.TOTL':
            data[row[1]]['population-total'] = row[4:]
        if row[3] == 'NY.GDP.MKTP.CD':
            data[row[1]]['gdp'] = row[4:]
        if row[3] == 'NY.GDP.MKTP.KD.ZG':
            data[row[1]]['gdp-growth'] = row[4:]
        if row[3] == 'SP.DYN.LE00.IN':
            data[row[1]]['life-expectancy'] = row[4:]
        if row[3] == 'SP.DYN.TFRT.IN':
            data[row[1]]['fertility'] = row[4:]


with open('./data.json', 'w') as outf:
    outf.write(json.dumps(data))
