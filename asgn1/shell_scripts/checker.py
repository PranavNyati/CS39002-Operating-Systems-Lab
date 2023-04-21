import sys
content = open(sys.argv[1]).read().replace('=', ' = ').replace('|', ' | ').replace(';', ' ; ').replace(',', ' , ').replace('<', ' < ').replace('>', ' > ')
print(content)
wordCount=len(content.split())
print(wordCount)