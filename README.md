CS179F
======

Operating System Project


Group Meetings:
Wednesday,October 29th: Discussed how we should work on representing directories, thought we could use the system calls mkdir to make physical linux directories.

Thursday,October 30th: We got together and talked about the overall structure of the our system and what needed to be done. Reviewed the what INodes were and how they were used in the Linux operating System.

Friday,October 7th: Went to lab, and realized that we aren't making physical directories in Linux. Instead a directory was literally just a map. So we decided to represent directories with a map inside the INode class. Got a good idea of what we were going to do and how to distinguish between files,directories,etc.

Wednesday, October 12th: At discussion, Professor Payne told us that we were not representing directories the way Linux does it, and that he had an example online to show us how Linux does it. We took a look at that example and agreed that it wasn't much different from ours and decided to follow that approach because it would be easier to find resources later down the road, as well as Professor's Payne idea of making our system as close to Linux as possible.

Thursday, October 13th: We meet up to look at the sample code that Professor Payne posted. We took some time to really understand what everything was doing. We then decided that having a CD command would be useful when testing if our filesystem worked so we started to write a cd function. After we got the basics of cd, we also worked on having the functionality of cd .. and cd.

Friday, October 14th: After some discussions with the professor and TA, we began fine tuning the code. We started working on knowing the parent and current INode. We also worked on a parser because we felt that it was needed to procede, even though the professor was working on one.

Sunday October 16th: We worked on understaind professor's Payne new updated verision of code and added some of our own that we had made from the previous verisions like the cd . and cd .. . We also added the command touch which allows the users to make a regular file. Also edited the mkdir function to have some functionality for mkdir -p which is if the user gives a pathname and the directories listed haven't been made yet, make them. i.e mkdir /a/b/c/ and there is no dir a,b,c it will make dirs a -> b -> c.

Tuesday October 18th: Met up and had some discussion on how to preserve the filesystem. We first thought a struct with a field for name,level,parent, and type would be the correct way to go. Then, we thought that we could use the unique paths. After awhile of debate, we finally realized that these structs are INodes so that we can just a data structure of INodes which have the paths and the info about the type with them.

Wednesday October 19th: We meet up and worked on printing out a tree for debugging purposes, similiar to the tree command on linux. We have it down pretty well but it needs a bit of fine tuning. Also, we discussed a bit about how we need to deal with Open File Tables, how to know when a file is open/closed, hardlinks, and also more about preserving a file system as the INode is an INode*. We also have to deal with devices which we are unsure of how to do right now.


Design Decisions:
1. Making directories a Map in INode -> having directories represented like in Unix.
2. Using the INodes to preserve filesystem(maybe write out the relevent fields and since we know how many fields we can read it back easily, but looking for alternatives)
3. How to deal with the open file system table
