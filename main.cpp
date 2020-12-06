/**************************************
 *     IMS Projekt                    *
 *                                    *
 *   Epidemiologické modely — makro   *
 *     Martin Škorupa  (xskoru00)     *
 *      Diana Barnová  (xbarno00)     *
 **************************************/

#include <iostream>
#include <random>
#include <cstdlib>
#include <getopt.h>

using namespace std;

struct probabilities_t {
	float getting_sick = 0.0;
	float healthy_staying_home = 0.0;
	float mild_symptoms = 0.0;
	float ms_staying_home = 0.0;
	float severe_symptoms = 0.0;
	float ss_staying_home = 0.0;

	float hospital_recovery = 0.0;
	float hospital_death = 0.0;
	float home_recovery = 0.0;
	float home_death = 0.0;

	float post_recovery_paranoia = 0.5;
} probability_of;

int percentage(){
   return min(rand() % 101, 100);  // ret random int in the range 0 to 100
}

class Population {
private:
	int* infected;
public:
	unsigned int total_population, incubation_period, is_infectious_since_day, average_daily_interactions, dead;
	unsigned int healthy_at_home, healthy_in_public;
	unsigned int asymptomatic_at_home, asymptomatic_in_public;
	unsigned int ms_at_home, ms_in_public;
	unsigned int ss_at_home, ss_waiting_for_bed, ss_in_bed;

	Population(unsigned int total_population,
				unsigned int incubation_period,
				unsigned int initial_number_of_sick,
				unsigned int is_infectious_since_day,
				unsigned int average_daily_interactions)
				{
		this->dead = this->healthy_at_home = this->healthy_in_public
			= this->asymptomatic_at_home = this->asymptomatic_in_public
			= this->ms_at_home = this->ms_in_public
			= this->ss_at_home = this->ss_waiting_for_bed = ss_in_bed = 0;
		this->incubation_period = incubation_period;
		this->is_infectious_since_day = is_infectious_since_day;
		this->average_daily_interactions = average_daily_interactions;
		this->total_population = total_population;

		this->infected = new int[this->incubation_period];
		this->infected[0] = initial_number_of_sick;
		for (int i = 1; i < this->incubation_period; i++){
			this->infected[i] = 0;
		}

	}
	
	void CalculateInteractions() {
		unsigned int available, available_infectious, available_mildly_infectious, present_infectious, present_healthy, picked_person;
		available = available_infectious = present_infectious = present_healthy = picked_person = 0;

		// Get people that are infectious, but don't know it yet (still within incubation period)
		for (unsigned int i = this->is_infectious_since_day ; i < this->incubation_period; i++){
			available_infectious += this->infected[i];
		}
		// Get people knowingly going around sick
		available_mildly_infectious = this->ms_in_public;
		// Get the number of all people in this interaction circle
		available = this->healthy_in_public + available_infectious + available_mildly_infectious;


		while (available) {
			// If there are no infectious left on this day, only healthy people meet and therefore this can be skipped
			if ((available_infectious + available_mildly_infectious) != 0) {
				// Pick a random combination of healthy and sick
				for (unsigned int i = 0; i < this->average_daily_interactions && available != 0; i++) {
					picked_person = rand() % available + 1;
					if (picked_person <= available_infectious) {
						--available;
						--available_infectious;
						++present_infectious;
					}
					else if (available_infectious < picked_person <= (available_infectious + available_mildly_infectious)){
						--available;
						--available_mildly_infectious;
						++present_infectious;
					}
					else {
						--available;
						++present_healthy;
					}
				}

				if (present_infectious) { // If interaction with at least one infectious person happened
					while (present_healthy) { // have a chance to affect all healthy people
						if (percentage() >= (int) (probability_of.getting_sick * 100)) {
							--this->healthy_in_public;
							++this->asymptomatic_in_public;
							++this->infected[0];
						}
						else {
							// If they get scared after finding out that
							if (percentage() >= (int) (probability_of.healthy_staying_home * 100)) {
								--this->healthy_in_public;
								++this->healthy_at_home;
							}
						}
						--present_healthy;
					}
				}
			}
			else { available = 0; }
		}
	}

	unsigned int GetNumberOfInfectedOnDay(int day){
		if (day > this->incubation_period) {
			return 0;
		}
		return this->infected[day-1];
	}

	void IllnessAdvances(){
		unsigned int past_incubation_period = this->infected[this->incubation_period];

		// Advance asymptotic infected one day forward
		for(unsigned int i = this->incubation_period; i >= 1; --i){
			this->infected[i] = this->infected[i-1];
		}
		this->infected[0] = 0;

		// Remove the ones past incubation period
		this->asymptomatic_in_public -= past_incubation_period;
		// and decide their fate
		while (past_incubation_period){
			if(percentage() <= (int)(probability_of.mild_symptoms*100)){ // Gain mild symptoms
				(percentage() <= (int)(probability_of.ms_staying_home*100)) ? ++this->ms_at_home : ++this->ms_in_public;
			}
			else { // Gain severe symptoms that require hospitalzation
				(percentage() <= (int)(probability_of.ss_staying_home*100)) ? ++this->ss_at_home : ++this->ss_waiting_for_bed;
			}

			--past_incubation_period;
		}
	}
	
	void Report(int day) const{
		cout << "========= REPORT ON DAY " << day << " ========="  << endl;
		cout << "Total population: " << this->total_population << endl
			 << " - infected:      " << this->total_population - this->dead - (this->healthy_at_home + this->healthy_in_public) << endl
			 << " - dead:          " << this->dead << endl;
		cout << "Healthy:          " << this->healthy_at_home + this->healthy_in_public << endl
			 << " - At home:       " << this->healthy_at_home << endl
			 << " - In public:     " << this->healthy_in_public << endl;
		cout << "Asymptomatic:     " << this->asymptomatic_at_home + this->asymptomatic_in_public << endl
			 << " - At home:       " << this->asymptomatic_at_home << endl
			 << " - In public:     " << this->asymptomatic_in_public << endl;
		cout << "Mild symptoms:    " << this->ms_at_home + this->ms_in_public << endl
			 << " - At home:       " << this->ms_at_home << endl
			 << " - In public:     " << this->ms_in_public << endl;
		cout << "Severe symptoms:                " << this->ss_at_home + this->ss_waiting_for_bed + this->ss_in_bed << endl
			 << " - At home:                     " << this->ss_at_home << endl
			 << " - Waiting for a hospital bed:  " << this->ss_waiting_for_bed << endl
			 << " - In a hospital bed:           " << this->ss_in_bed << endl;
		cout << "========== END OF REPORT ==========" << endl;
	}

};

void parseArguments(int argc, char* argv[]){

}

int main(int argc, char* argv[]) {
	unsigned int total_population = 10000000;
	int incubation_period = 10;
	int initial_number_of_sick = 150;
	int is_infectious_since_day = 1;
	int average_daily_interactions = 6;

	Population population = Population(total_population, incubation_period, initial_number_of_sick, is_infectious_since_day, average_daily_interactions);

	population.CalculateInteractions();


	/* After each day the infected in incubation period advance to the next day.
	 * If they're past the incubation period, the next day they don't meet with
	 * anyone and either stay home or try to get admitted into the hospital to get treatment*/
	population.IllnessAdvances();
	return 0;
}
