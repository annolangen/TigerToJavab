SEP := $(shell [[$(uname) == CYGWIN*]] && echo ';' || echo :)
run: ../Std.class Main.class
	java -cp ".$(SEP)..' Main

show: Main.class
	javap -v -c Main

%.class: %.java
	javac -g:none -cp ".$(SEP).." $<

Main.class: ../Std.class

clean:
	rm *.class ../*.class

.PHONY: run clean show

