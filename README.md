# acotsp
Ant Colony Optimization for TSP

This is the improved version based on the copy from http://www.aco-metaheuristic.org/aco-code, ACOTSP.V1.03.tgz
Follow ACO variants are support:
1. Basic ACO;
2. Elite ACO;
3. Rank-based ACO;
4. Max-min ACO;
5. Best-worest ACO;
6. Ant Colony System(ACS);
7. Levy Flight ACO;
8. Contribution-based ACO;
Based on our experiments, The performance sequence is ACS > Rank-based ACO > Max-min ACO > Elite ACO > Basic ACO > Best-worest ACO; Levy Flight ACO and Contribution-base ACO can improve above ACOs.

Improved points:
1. Add Function to output optimization result for acotsp in CSV format which is easy for later anaylse.
2. Add Improved Function for Epsilon Greedy policy to ACO;
3. Add Improved Function for candidators selection mechanism of Levy Flight;
4. Add Improved Function for pheromone update logic based on the contribution of each arc in the solution.

Usage:
1. Basic parameters is similar with original acotsp, please run acotsp --help for details.
2. For Greedy Levy Flight ACO, parameters  -G x:y:z is used.
        x means Epsilon Greedy Threshold which value between 0 to 1, suggest value is 0.7; 
        y means Levy Flight Threshold which value between 0 to 1, suggest value is 0.8-0.9; 
	z means Levy Flight Alter Ratio great than 0, suggest value is 4;
	please run acotsp --help for details with Epsilon Greedy Levy Flight;
3. For Contribution-based ACO, parameter -C x is used.
    x means Contribution effect alter ratio, suggest value is 4.
4. Usage for acotsp please refer the demo shell file in shell directory.
   run.bat is used for Windows/Dos;
   run.sh is used for Linux;

Files:
1. tsp: TSP instances for acotsp to run, please refer the files in shell directory.
2. *.c *.h source code for ACO
3. makefile_dos, makefile_linux Makefile to building ACO;
4. *.sh Linux Shell to run ACO;
5. *.bat Windows/DOS batch file to run ACO.

Instructor for running ACOTSP
1. Clone by git to get the source code of this project;
2. Download or install gcc for the computer based on its OS.
   For windows, https://sourceforge.net/projects/mingw-w64-builds/ is suggested, and mingw or mingw64 is alternative options for experienced users.
   For linux, gcc is needed, run sudo apt-get g++ or sudo apt-get build-essential in Ubuntu, other Linux please refer the manual.
3. Complie the source code in src directory;
   mingw32-make -f makefile_dos For Windows;
   make -f makefile_linux for Linux;
   copy or move acotsp to tsp directory before run the scipts.
4. Copy or Modify the run script file;
   run.bat for Windows; run.sh for Linux;
5. Edit the run script before run;
6. Update the TSP instance and its optimal result, which can get from https://www.iwr.uni-heidelberg.de/groups/comopt/software/TSPLIB95/STSP.html
7. Run the scipt and analyst the output file in CSV files whose name in the run script;
