run: ../Std.class Main.class
	java -cp .:.. Main

show: Main.class
	javap -v -c $<

%.class: %.java
	javac -g:none -cp .:.. $<

clean:
	rm *.class ../*.class

.PHONY: run clean show
