import csv
import json

WRITE_CSV = False

country_popul = {}
with open('./data.csv', 'r') as inf, open('./map-data.csv', 'w') as outf:
    reader = csv.reader(inf, delimiter=',', quotechar='"')
    if WRITE_CSV:
        writer = csv.writer(outf, delimiter=',', quotechar='"',
                            quoting=csv.QUOTE_MINIMAL)
        writer.writerow(['country_code'] + ['_' + str(i)
                                            for i in range(2020, 1959, -1)])
    for row in reader:
        if row[3] == 'SP.POP.1564.TO.ZS':
            country_popul[row[1]] = row[4:]
            if WRITE_CSV:
                writer.writerow([row[1]] + row[4:])

with open('./map-data.json', 'w') as outf:
    outf.write(json.dumps(country_popul))







