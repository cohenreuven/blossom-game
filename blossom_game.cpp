#include <stdlib.h>
#include <iostream>
#include <math.h>

// number of days incl. 0=stay at home
#define N_DAYS 20

//number of players
#define N_PLAYERS 2000

// number of steps
#define N_STEPS 100

// information yes/no
#define INFO 1

// minimum utility. below that utility is negative
#define MIN_UTIL 0.1

//value of negative utility
#define NEG_UTIL -1

// probability of each player to be of type K1
#define PROB_K1 0.0

//heterogeneity in preference
#define HETERO 2.0

//number of square roots to take as cautiousness factor
#define SIGMA 5

//factor for probabilty to stay at home after disappointment (learning rate)
#define FACTOR 0.5

// initial probability of staying at home after disappointment
#define P_HOME 0.5

//probability to replace each player with a new one each season
#define P_REPLACE 0.0

using namespace std;

// decide with probability prob
inline int choose_prob(double prob)
{
	return (rand()<prob*RAND_MAX);
}

//real random number in range [0,1)
inline double rand_real()
{
	return (rand()/(RAND_MAX+1.0));
}

// choose randomly from distribution
int choose_prob(double prob[])
{
	int i,random=rand();
	for (i=0;random>prob[i]*RAND_MAX;++i) 
	{
	}
	return i;
}

class blossom
{
	public:
		int flowers[N_DAYS];
				
		//initialize daily blossom
		blossom()
		{
			int i;
			flowers[0]=0;

			for (i=1;i<N_DAYS;++i)
			{
				flowers[i]=i; //blossom increases on first half of days
				if (i*2>N_DAYS) flowers[i]=N_DAYS-i; //declining blossom on second half
			}
		}
};

class player
{
	private:
		int day;
		int K;
		blossom *my_blossom;
		double utility;
		double my_pref; // player's preference
		double p_leave; // probability to leave home
		double dist[N_DAYS];
		int change;
	public:	
		inline void set_K(int new_K) {K=new_K;}
		inline int get_K() {return K;}
		inline double get_utility() {return utility;}
		inline double get_pref() {return my_pref;}
		inline void set_pref(double pref) {my_pref=pref;}
		inline int get_day() {return day;}
		inline int get_change() {return change;}
		
		void calc_utility(int attendence[])
		{
			if (day==0) utility=0;
			else
			{
				utility=my_blossom->flowers[day]*my_pref/attendence[day];
				if (utility<MIN_UTIL) utility=NEG_UTIL;
			}
		}
		
		init(blossom *bloss)
		{
			int i;
			my_blossom=bloss;
			//initially nobody stays home;
			dist[0]=0;
			// choose day randomly according to blossom distribution
			for (i=1;i<N_DAYS;++i)
			{
				//commulative number of flowers
				dist[i]=dist[i-1]+bloss->flowers[i];
			}
			// normalize distribution
			for (i=1;i<N_DAYS;++i)
			{
				dist[i]=dist[i]/dist[N_DAYS-1];
			}
			dist[N_DAYS-1]=1.0;
			
			// randomly choose day
			day=choose_prob(dist);
			//init pprobability of leaving home to 1
			p_leave=1-P_HOME;
		}
		
		//make a decision without info
		void play()	
		{
			// with probability P_REPLACE start from scratch
			if (choose_prob(P_REPLACE) )
			{
				init(my_blossom);
				return;
			}
			change=0;
			if (utility>0) return;

			if (utility<0) 
			{
			    p_leave=p_leave*FACTOR; //decrease the probability to leave home
			    change=1;
			}
		
			if (!choose_prob(p_leave)) //stay at home with probability 1-p_leave
			{
				day=0;
				return;
			}
			change=1;
			day=choose_prob(dist); // choose day based on blossom distribution
		}	

		inline double calc_util(int day, int attendence[])
		{
			double temp;
			temp=my_blossom->flowers[day]*my_pref/(attendence[day]+1+K*SIGMA*sqrt(attendence[day]));
			if (temp<MIN_UTIL) temp=NEG_UTIL;
			return temp;
		}
		
		//make a decision with info
		void play(int attendence[])	
		{
			int i,j,k;
			int is_max[N_DAYS],max_days=1;
			double max=0,util;
			change=0;
			// with probability P_REPLACE start from scratch
			if (choose_prob(P_REPLACE) )
			{
				init(my_blossom);
				return;
			}
			calc_utility(attendence);
			//if player was happy last step - no need to change
			if (utility>0) return;
			if (utility<0) 
			{
                p_leave=p_leave*FACTOR; //decrease the probability to leave home
                change=1;
                
            }
			
			if (!choose_prob(p_leave)) //stay at home with probability 1-p_leave
			{
				day=0;
				return;
			}
			
			is_max[0]=1; //initially the best possibility is stay at home
			// check for better days
			for (i=1;i<N_DAYS;++i)
			{
				util=calc_util(i,attendence);
				// day i is the new best day
				if (util>max) 
				{
					max=util;
					is_max[i]=1;
					max_days=1;
					for (j=0;j<i;++j) is_max[j]=0; //delete all previous best days
				}
				//another day with the maximum utility
				else if (util==max)
				{
					is_max[i]=1;
					max_days++;
				}
				else is_max[i]=0;
			}
			//choose one of the best days randomly
			k=rand()%max_days+1;
			j=0;
			for (i=0;j<k;j+=is_max[i++]) ;
						
			day=i-1;
			if (day>0) change=1;
		}	
	
};


class game
{
	private:
		blossom game_blossom;
		double avg_util[N_DAYS],avg_pref[N_DAYS],min_pref[N_DAYS],max_pref[N_DAYS],util_stat[N_DAYS][N_STEPS],change_stat[N_STEPS];
		int tot_players[N_DAYS],players_stat[N_DAYS][N_STEPS];
		player players[N_PLAYERS];
		
	public:
		game()
		{
			int i;
			for (i=0;i<N_PLAYERS;++i)
			{
				// init the type of each player to K0 or K1 (woth probability PROB_K1)
				players[i].set_K(choose_prob(PROB_K1));
				//init the players preference
				players[i].set_pref(1+HETERO*rand_real());
				players[i].init(&game_blossom);
			}
		}

		void calc_stat()
		{
			int i;
			for(i=0;i<N_DAYS;++i)
			{
				tot_players[i]=0;
				avg_util[i]=0;
				avg_pref[i]=0;
				min_pref[i]=0;
				max_pref[i]=0;
			}

			for(i=0;i<N_PLAYERS;++i)
			{
				tot_players[players[i].get_day()]++;
				avg_util[players[i].get_day()]+=players[i].get_utility(); //total utility for day i
				avg_pref[players[i].get_day()]+=players[i].get_pref(); //total utility for day i
				if (min_pref[players[i].get_day()]==0)
				{
					min_pref[players[i].get_day()]=min_pref[players[i].get_day()]=players[i].get_pref();
				}
				else
				{
					if (min_pref[players[i].get_day()]>players[i].get_pref())
					{
						min_pref[players[i].get_day()]=players[i].get_pref();
					}
					if (max_pref[players[i].get_day()]<players[i].get_pref())
					{
						max_pref[players[i].get_day()]=players[i].get_pref();
					}
				}
			}
			
			
			for(i=0;i<N_DAYS;++i)
			{
				if (tot_players[i]>0) 
				{
					avg_util[i]/=tot_players[i]; //divide by num of participants to get average
					avg_pref[i]/=tot_players[i]; //divide by num of participants to get average	
				}
			}
			
		}

		void calc_stat(int step)
		{
			int i;
			change_stat[step]=0;
			
			for(i=0;i<N_DAYS;++i)
			{
				players_stat[i][step]=0;
				util_stat[i][step]=0;
			}

			for(i=0;i<N_PLAYERS;++i)
			{
				players_stat[players[i].get_day()][step]++;
				util_stat[players[i].get_day()][step]+=players[i].get_utility(); //total utility for day i
				change_stat[step]+=players[i].get_change();
			}
			
			
			for(i=0;i<N_DAYS;++i)
			{
				if (players_stat[i][step]>0) util_stat[i][step]/=players_stat[i][step]; //divide by num of participants to get average
			}
			
		}


		// simulate n_steps steps of the game
		void steps(int n_steps)
		{
			int i,j;
			for (i=0;i<n_steps;++i)
			{
				// each player makes decision
				for (j=0;j<N_PLAYERS;++j)
				{
					if (INFO)
					{
						players[j].play(tot_players);
					}
					else
					{
						players[j].play();
					}

				}
				
				for(j=0;j<N_DAYS;++j)
				{
					tot_players[j]=0;
				}
				//calculate total attendence
				for (j=0;j<N_PLAYERS;++j)
				{
					tot_players[players[j].get_day()]++;
				}
			
				//calculate each palyers utility
				for (j=0;j<N_PLAYERS;++j)
				{
					players[j].calc_utility(tot_players);
				}

				calc_stat(i);
			}
			

		}		

		void print_stat()
		{
			int i;
			calc_stat();
			//print day number, number of players for that day, average utility, average preference of palyers for that day
			for (i=0;i<N_DAYS;++i) cout<<i<<","<<tot_players[i]<<","<<avg_util[i]<<","<<avg_pref[i]<<","<<min_pref[i]<<","<<max_pref[i]<<endl;
		}

		void print_history()
		{
			int i,j;
			//print day number, number of players for that day, average utility of palyers for that day
			for (i=0;i<N_DAYS;++i)
			{
				cout<<i<<",";
				for (j=0;j<N_STEPS;++j) cout<<players_stat[i][j]<<",";
				cout<<endl;
			}
			cout<<endl;
			for (i=0;i<N_DAYS;++i)
			{
				cout<<i<<",";
				for (j=0;j<N_STEPS;++j) cout<<util_stat[i][j]<<",";
				cout<<endl;
			}

			cout<<endl;
			for (i=0;i<N_STEPS;++i)
			{
				cout<<change_stat[i]<<",";
			}
			cout<<endl;
			
		}
};

// check if seed is default
int main()
{
	game new_game;
	// run simulation:
	new_game.steps(N_STEPS);
	//print all history:
	new_game.print_history();
	//print last state:
//	new_game.print_stat();
	return 0;
}
