# Description for bash script
flex -olex.ExpressionParser.c < ExpressionParser.l
bison -oExpressionParser.tab.c ExpressionParser.y
