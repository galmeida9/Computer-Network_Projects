Project directories:

    client/: Stores all the files for the client, every topic information that the client gets from the server, it stores
            that topic's folder inside the client directory;

    lib/: Collection of utility functions and macros with general purpose;

    topics/: Contains the server's topic folders and the List_of_Topics file, that contains the list of every topic and who requested it;
	
	topics/RC/: demonstration files, to demonstrate how the server organizes the information;
	
	scripts/: Has all the script results, each one in a .pdf file.

--------------------------------------------------------------------------------------------------------------------------------------------

How to compile the project:
    make - compiles both the server and the client with the name FS and user respectively on the root folder;
    make clean - erases every compiled file from the project.

--------------------------------------------------------------------------------------------------------------------------------------------

List_of_Topics.txt file:
    
    When adding a topic to the server, it adds the topic name and the user ID of who requested it into this file which is located in the 
    client directory, with the following syntax:
            topic_name:userID

            example:
                RC:56789
                IPnetwork1:12345
                IPnetwork2:12345
                IPnetwork3:12345

    Note: the file always ends with a paragraph.

--------------------------------------------------------------------------------------------------------------------------------------------

_questions.txt file:

    When adding a topic to the server it creates this file in the topic directory, the file initialy its empty. 
    After adding questions information its added to the file with the following syntax:
            if there is no image:
                name_of_question:userID:numb_of_answers:0:
            otherwise:
                name_of_question:userID:numb_of_answers:1:image_ext:

            example: 
                pergunta:89446:03:1:jpg:
    
    Note: the numb_of_answers its always with 2 digits, so if the number of answers of a given question is 2, numb_of_answers
          the file its displayed 02, the file always ends with a paragraph.

--------------------------------------------------------------------------------------------------------------------------------------------

"name_of_answer"_desc.txt:
    For every answer the file  - name_of_answer_desc.txt its created on the answer's topic directory, the following way:
            if there is no image:
                userID:0:
            otherwise:
                userID:1:image_ext:

--------------------------------------------------------------------------------------------------------------------------------------------

students.txt file:
    Located in the root directory and has the numbers of the user IDs that are allowed to register.
        
        example:   
            12345
            89568
            54321
            56789
            77777

--------------------------------------------------------------------------------------------------------------------------------------------

Variables meaning:
    image_ext - the extention of the image file
    name_of_answer - name of the answer file (e.g.: pergunta_02)
    name_of_question - name of the question
    numb_of_answers - number of answers with 2 digits
    topic_name - name of the topic
    userID - the ID of the person who submited or requested