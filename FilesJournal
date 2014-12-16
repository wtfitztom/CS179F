CS179F
======

Operating System Project


Group Meetings:
Wednesday,October 29th: Discussed how we should work on representing directories, thought we could use the system calls mkdir to make physical linux directories.

Thursday,October 30th: We got together and talked about the overall structure of the our system and what needed to be done. Reviewed the what INodes were and how they were used in the Linux operating System.

Friday,Novemeber 7th: Went to lab, and realized that we aren't making physical directories in Linux. Instead a directory was literally just a map. So we decided to represent directories with a map inside the INode class. Got a good idea of what we were going to do and how to distinguish between files,directories,etc.

Wednesday, November 12th: At discussion, Professor Payne told us that we were not representing directories the way Linux does it, and that he had an example online to show us how Linux does it. We took a look at that example and agreed that it wasn't much different from ours and decided to follow that approach because it would be easier to find resources later down the road, as well as Professor's Payne idea of making our system as close to Linux as possible.

Thursday, November 13th: We meet up to look at the sample code that Professor Payne posted. We took some time to really understand what everything was doing. We then decided that having a CD command would be useful when testing if our filesystem worked so we started to write a cd function. After we got the basics of cd, we also worked on having the functionality of cd .. and cd.

Friday, Novemeber 14th: After some discussions with the professor and TA, we began fine tuning the code. We started working on knowing the parent and current INode. We also worked on a parser because we felt that it was needed to procede, even though the professor was working on one.

Sunday November 16th: We worked on understaind professor's Payne new updated verision of code and added some of our own that we had made from the previous verisions like the cd . and cd .. . We also added the command touch which allows the users to make a regular file. Also edited the mkdir function to have some functionality for mkdir -p which is if the user gives a pathname and the directories listed haven't been made yet, make them. i.e mkdir /a/b/c/ and there is no dir a,b,c it will make dirs a -> b -> c.

Tuesday November 18th: Met up and had some discussion on how to preserve the filesystem. We first thought a struct with a field for name,level,parent, and type would be the correct way to go. Then, we thought that we could use the unique paths. After awhile of debate, we finally realized that these structs are INodes so that we can just a data structure of INodes which have the paths and the info about the type with them.

Wednesday Novemberr 19th: We meet up and worked on printing out a tree for debugging purposes, similiar to the tree command on linux. We have it down pretty well but it needs a bit of fine tuning. Also, we discussed a bit about how we need to deal with Open File Tables, how to know when a file is open/closed, hardlinks, and also more about preserving a file system as the INode is an INode*. We also have to deal with devices which we are unsure of how to do right now.

Thursday, November 20th: We tried to implement our write out to file to preserve our filesystem. We figured out how to store the path.

Friday, November 21th: We were able to read back in the file and reconstruct the filesystem, but we couldn't read/write to files so only the structures were being preserved.

Tuesday, November 25: We met to try to work on allowing the user to read/write to the files. We thought of the idea to just add a string for the text in the File class and when you want to write to a file you set the that string and that is "writting." We were able to achieve this, and reading.

Wednesday, November 26th: We met again to try to be able to preserve the text inside of a files because we were currently only restoring the files and not the text that was inside it. It took us a long time, but we were finally able to save the text of the file but we were unable to get it read back in to restore it back.

Friday, November 28th: My, William, and myself worked online on it to try to get it to restore the text. We realized that we were over complicating it the last time so we scratched that and started again. And we got it to work so that our entire file system was preserved.

Wednesday December 3rd: Thought about how we could intergrate our code with the other groups because shell and thread had already intergrated their code. We thought it would be simple if the shell team was able to parse tokens into a vector. So we met with the shell team after discussion and told them our plans and they said they had done that already, so we were set to try to intergrate our stuff with theirs.

Friday December 5th: We spent most of the time in the beginnging of lab cleaning up our code and getting it ready to be intergrated with the other teams, i.e making a namespace, seperating our code into a .h and .cpp, etc. Then we spent the later half of the lab with the shell and threads team trying to ingergate the shell with our filesystem. It took some time to coordinate our naming conventions as we did not really do it througout the development process. Once we got it to compile, we saw that the filesystem functions that did not require arguement passing worked flawless(ls,tree) but any command that required arguement passing did not work. So we went into our code and tried debugging it by printing out the arguements in our file system code and we saw that it was recieving the correct arguements. Both our teams were stumped, but our team kept trying to debug and we realized that it was working in our thread, but it was somehow not being passed back to the main thread. This is not an issue on our side, so we told the threads team and they looked into the issue.


Wednesday December 10th: The threads/shell team came to us and told us that they could not really solve the problem so our three teams met and tried to figure out the problem together. Patrick finally saw the problem and realized that it was because the shell team was calling fork, which created another copy which isn't what we wanted so the shell team went to work on it.

Friday December 12th: We realized that the professor wanted us to have commands cp and mv which were basic operations, so we spent most of our time implementing this. We got both to work pretty similiarly to how Linux does the commands. We also found that the shell/thread team fixed the problem so our filesystem was fully intergrated with those other 3 teams. Towards the end of the lab, the devices team came to us and asked us to intergrate their stuff with our file system. Both of our teams were confused as how we go about doing this, and the Professor had already left so we spent a good amount of time talking to each about what their things to and bouncing ideas off of one another. Eventually, we came to the conclusion that the devices team should rewrite some of their code using our iNodes so that we can intergrate them cleanly. We are awaiting there updated code.
Monday December 15th: We started working on an function for opening/closing files so that the shell team could handle IO redirection but we did not make much progress and we have other things on our plates so we have decide that we will get this part if we have some left over time from finishing finals early.

Tuesday December 16th: We have not heard back from the devices team and so we do not think we will have time to intergrate their things wiht our system. We are currently busy studying and working on projects for our finals week and we still have some functions that we need to make for the shell team to do IO redirection. However, we are extremely busy at this time and it is highly likely that we will not able to get this done in time.

Design Decisions:
1. Making directories a Map in INode -> having directories represented like in Unix.
2. Using the INodes to preserve filesystem(maybe write out the relevent fields and since we know how many fields we can read it back easily, but looking for alternatives)
3. We tried to make our functionality as code as simple as possible so that everyone could easily understand it and pick it up if they have basic understanding of OOP.
4. We also tried to model our functionality off of Linux as much as possible. For example, when you type mv if it does not exist in Linux it'll make you that file, so ours does too. There are some cases where Linux doesn't make the new file or dir so ours doesn't do it either.(too the best of our knowledge)
