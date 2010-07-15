import re, sys
print len(max(re.findall('\x00+', open(sys.argv[1]).read())))
