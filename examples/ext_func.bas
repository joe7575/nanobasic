for i = 1 to 100
    clrscr()
    setcur(1,1)
    print "t = "; time()
    name$ = input$("Your name")
    print "Hello "; name$
    age = input("What is your age?")
    print "Next year you will be" age+1 "years old"
    sleep(4)
next i
