/**********************************************
 *                 IMS Project                *
 *                                            *
 *   Epidemiological macro model simulation   *
 *         Martin Škorupa  (xskoru00)         *
 *          Diana Barnová  (xbarno00)         *
 **********************************************/

#include <iostream>
#include <random>
#include <cstdlib>
#include <getopt.h>

using namespace std;

bool debugging_enabled = false;
#define DEBUG(msg) do { \
  if (debugging_enabled) { msg } \
} while (0)

unsigned int* incubating;

struct probabilities_t {
	float getting_sick = 0.25;
	float healthy_staying_home = 0.50;
	float mild_symptoms = 0.70;
	float ms_staying_home = 0.25;

	float hospital_recovery = 0.0;
	float hospital_death = 0.0;
	float home_recovery = 0.0;
	float home_death = 0.0;

	float post_recovery_paranoia = 0.5;
} probability_of;

// Returns 0.0001 (0.01%) to 1.0 (100%)
float percentageFraction(){
   return (float)(min(rand() % 10001, 10000))/(float)10000;
}

class Population {
public:
	unsigned int day;
	unsigned int total_population, incubation_period, is_infectious_since_day, average_daily_interactions, dead;
	unsigned int healthy_at_home, healthy_in_public;
	unsigned int asymptomatic_at_home, asymptomatic_in_public;
	unsigned int ms_at_home, ms_in_public;
	unsigned int ss_waiting_for_bed, ss_in_bed;
	unsigned int available_hospital_beds;

	Population(unsigned int total_population,
				unsigned int incubation_period,
				unsigned int initial_number_of_sick,
				unsigned int is_infectious_since_day,
				unsigned int average_daily_interactions,
				unsigned int number_of_hospital_beds)
				{
		this->day = this->dead = this->healthy_at_home
			= this->asymptomatic_at_home
			= this->ms_at_home = this->ms_in_public
			= this->ss_waiting_for_bed = this->ss_in_bed = 0;

		this->total_population = total_population;
		this->incubation_period = incubation_period-1;
		this->is_infectious_since_day = is_infectious_since_day-1;
		this->average_daily_interactions = average_daily_interactions;
		this->healthy_in_public = total_population - initial_number_of_sick;
		this->available_hospital_beds = number_of_hospital_beds;
		this->asymptomatic_in_public = initial_number_of_sick;
	}
	
	void CalculateInteractions(bool local_debug_out_enabled = false) {
		local_debug_out_enabled ? debugging_enabled = true : debugging_enabled = false;

		unsigned int available, available_infectious, available_mildly_infectious, present_infectious, present_healthy, picked_person, x;
		available = available_infectious = available_mildly_infectious = present_infectious = present_healthy = picked_person = x = 0;

		DEBUG(cout << "Infection spreading events: " << endl;);
		// Get people that are infectious, but don't know it yet (still within incubation period)
		for (unsigned int i = this->is_infectious_since_day; i <= this->incubation_period; i++){
			available_infectious += incubating[i];
			DEBUG(cout << "I|  Incubated for " << i << " days: " << incubating[i] << endl;);
		}
		// Get people knowingly going around sick
		available_mildly_infectious = this->ms_in_public;
		// Get the total number of people in this interaction circle
		available = this->healthy_in_public + available_infectious + available_mildly_infectious;

		DEBUG(cout << "I| Infectious in incubation: " << available_infectious << endl;);
		DEBUG(cout << "I| Mildly infectious: " << available_mildly_infectious << endl;);
		DEBUG(cout << "I| Available people: " << available << " of which " << this->healthy_in_public << " are healthy." << endl;);

		while (available) {

			// If there are no healthy or no infectious left on this day, only healthy people meet and therefore this can be skipped
			if ((available - (available_infectious + available_mildly_infectious)) != 0
					&& (available_infectious + available_mildly_infectious) != 0) {
				present_healthy = present_infectious = 0;

				// Pick a random combination of healthy and sick
				for (unsigned int i = 0; i < this->average_daily_interactions && available != 0; i++) {
					picked_person = rand() % available + 1;
					if (picked_person <= available_infectious) {
						--available;
						--available_infectious;
						++present_infectious;
						DEBUG(cout << "I|  Picked a person number " << picked_person << " of " << available+1 << " that is infectious." << endl;);
					}
					else if (available_infectious < picked_person && picked_person < (available_infectious + available_mildly_infectious)){
						--available;
						--available_mildly_infectious;
						++present_infectious;
						DEBUG(cout << available_infectious << " " << picked_person << " " << (available_infectious + available_mildly_infectious) << endl;);
						DEBUG(cout << "I|  Picked a person number " << picked_person << " of " << available+1 << " that is mildly infectious." << endl;);
					}
					else {
						--available;
						++present_healthy;
						DEBUG(cout << "I|  Picked a person number " << picked_person << " of " << available+1 << " that is healthy." << endl;);
					}
				}

				DEBUG(cout << "I| (" << x << ") | Present healthy: " << present_healthy << " | Present infectious: " << present_infectious << " | " << endl;);

				// If interaction with at least one infectious person happened
				if (present_infectious) {
					// have a chance to affect all healthy people
					while (present_healthy) {
						if (percentageFraction() <= probability_of.getting_sick) {
							--this->healthy_in_public;
							++this->asymptomatic_in_public;
							++incubating[0];
							DEBUG(cout << "I|   - Became asymptomatic" << endl;);
						}
						else {
							// If they get scared after finding out that
							if (percentageFraction() <= probability_of.healthy_staying_home) {
								--this->healthy_in_public;
								++this->healthy_at_home;
								DEBUG(cout << "I|   - Healthy going home" << endl;);
							}
							else{
								DEBUG(cout << "I|   - Healthy staying in public" << endl;);
							}
						}
						--present_healthy;
					}
				}
				DEBUG(cout << "I| -- Unevaluated people left: " << available << " of which healthy: " << available - (available_infectious + available_mildly_infectious) << " --" << endl;);
			}
			else { available = 0; }
			++x;
		}

		local_debug_out_enabled ? debugging_enabled = false : debugging_enabled = true;
	}

	void Hospital(bool local_debug_out_enabled = false){
		local_debug_out_enabled ? debugging_enabled = true : debugging_enabled = false;
		DEBUG(cout << "Hospital events: " << endl;);

		DEBUG(cout << "H| Start evaluating patients:" << endl;);
		// Attempt to release patients to increase intake capacity
		for (unsigned int i = 1; i <= this->ss_in_bed; i++) {
			float patients_fate = percentageFraction();

			DEBUG(cout << "H|  - Patient " << i;);
			// Patient recovers
			if(patients_fate <= probability_of.hospital_recovery){
				++this->available_hospital_beds;
				--this->ss_in_bed;
				DEBUG(cout << " has recovered and ";);
				// The recovered patient is paranoid and goes home until this ends
				if(percentageFraction() <= probability_of.post_recovery_paranoia){
					++this->healthy_at_home;
					DEBUG(cout << "has post recovery paranoia (Self quarantine)." << endl;);
				}
				// The recovered patient feels good and goes on with his normal life
				else{
					++this->healthy_in_public;
					DEBUG(cout << "is returning into public." << endl;);
				}
			}
			// Patient dies
			else if(probability_of.hospital_recovery < patients_fate
					&& patients_fate <= (probability_of.hospital_recovery + probability_of.hospital_death)){
				++this->available_hospital_beds;
				--this->ss_in_bed;
				++this->dead;
				DEBUG(cout << " has died." << endl;);
			}
			// Patient stays for another day
			else{
				DEBUG(cout << " continues their stay at the hospital." << endl;);
				// Nothing changes
			}
		}

		DEBUG(cout << "H| Start admitting patients:" << endl;);
		// There is enough available beds so all people are admitted
		if(this->ss_waiting_for_bed <= this->available_hospital_beds){
			this->available_hospital_beds -= this->ss_waiting_for_bed;
			DEBUG(cout << "H|  - All (" << this->ss_waiting_for_bed << ") patients waiting for a bed were admitted to the hospital. Unoccupied hospital beds left: " << this->available_hospital_beds << endl;);
			this->ss_in_bed += this->ss_waiting_for_bed;
			this->ss_waiting_for_bed = 0;
		}
		// There is not enough available beds so some people are left waiting
		else{
			this->ss_waiting_for_bed -= this->available_hospital_beds;
			DEBUG(cout << "H|  - Some (" << this->available_hospital_beds << ") patients were admitted to the hospital. Patients still waiting: " << this->ss_waiting_for_bed << endl;);
			this->ss_in_bed += this->available_hospital_beds;
			this->available_hospital_beds = 0;
		}

		local_debug_out_enabled ? debugging_enabled = false : debugging_enabled = true;
	}

	void HomeQuarantine(bool local_debug_out_enabled = false){
		local_debug_out_enabled ? debugging_enabled = true : debugging_enabled = false;

		DEBUG(cout << "Home self quarantine events: " << endl;);
		for(unsigned int i = 1; i <= this->ms_at_home; i++){
			float fate = percentageFraction();

			// Person recovers at home an returns into public
			if(fate <= probability_of.home_recovery){
				--this->ms_at_home;
				++this->healthy_in_public;
				DEBUG(cout << "Q| - Person " << i << " has recovered and returns to public." << endl;);
			}
			// Person dies at home
			else if(probability_of.home_recovery < fate
					&& fate <= (probability_of.home_recovery + probability_of.home_death)){
				--this->ms_at_home;
				++this->dead;
				DEBUG(cout << "Q| - Person " << i << " has died in home quarrantine." << endl;);
			}
			// Person's status has worsened and needs medical attention
			else{
				--this->ms_at_home;
				++this->ss_waiting_for_bed;
				DEBUG(cout << "Q| - Person " << i << " needs medical attention and is now waiting for a hospital bed." << endl;);
			}
		}

		local_debug_out_enabled ? debugging_enabled = false : debugging_enabled = true;
	}

	void IllnessAdvances(bool local_debug_out_enabled = false){
		local_debug_out_enabled ? debugging_enabled = true : debugging_enabled = false;

		unsigned int past_incubation_period = incubating[this->incubation_period];

		DEBUG(cout << "Illness advancing events: " << endl;);
		// Advance asymptotic incubating one day forward
		DEBUG(cout << "A| | " << incubating[0] << " | " << incubating[1] << " | " << incubating[2] << " | " << incubating[3] << " | " << incubating[4] << " | " << incubating[5] << " | " << incubating[6] << " | " << incubating[7] << " | " << incubating[8] << " | " << incubating[9] << " |" << endl;);
		for(unsigned int i = this->incubation_period; i >= 1; --i){
			incubating[i] = incubating[i - 1];
			DEBUG(cout << "A| | " << incubating[0] << " | " << incubating[1] << " | " << incubating[2] << " | " << incubating[3] << " | " << incubating[4] << " | " << incubating[5] << " | " << incubating[6] << " | " << incubating[7] << " | " << incubating[8] << " | " << incubating[9] << " |" << endl;);
		}
		incubating[0] = 0;
		DEBUG(cout << "A| | " << incubating[0] << " | " << incubating[1] << " | " << incubating[2] << " | " << incubating[3] << " | " << incubating[4] << " | " << incubating[5] << " | " << incubating[6] << " | " << incubating[7] << " | " << incubating[8] << " | " << incubating[9] << " |" << endl;);

		// Process people newly past the incubation period
		this->asymptomatic_in_public -= past_incubation_period;
		DEBUG(cout << "A| Past incubation period: " << past_incubation_period << endl;);
		// and decide their fate
		while (past_incubation_period){
			if(percentageFraction() <= probability_of.mild_symptoms){ // Gain mild symptoms
				DEBUG(cout << "A|  Got mild symptoms - At home/In public " << this->ms_at_home << "/" << this->ms_in_public << " => ";);
				(percentageFraction() <= probability_of.ms_staying_home) ? ++this->ms_at_home : ++this->ms_in_public;
				DEBUG(cout << this->ms_at_home << "/" << this->ms_in_public << endl;);
			}
			else { // Gain severe symptoms that require hospitalization
				DEBUG(cout << "A|  Got severe symptoms and is now waiting for a hospital bed." << endl;);
				++this->ss_waiting_for_bed;
			}

			--past_incubation_period;
		}

		local_debug_out_enabled ? debugging_enabled = false : debugging_enabled = true;
	}

	void Report() const{
		cout << "========= REPORT ON DAY " << this->day << " ========="  << endl;
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
		cout << "Severe symptoms:                " << this->ss_waiting_for_bed + this->ss_in_bed << endl
			 << " - Waiting for a hospital bed:  " << this->ss_waiting_for_bed << endl
			 << " - In a hospital bed:           " << this->ss_in_bed << endl;
		cout << "========== END OF REPORT ==========" << endl;
	}

};

//void parseArguments(int argc, char* argv[]){

//}

int main(int argc, char* argv[]) {
	bool local_debugging_enabled = true;

	unsigned int number_of_simulation_days = 14;

	unsigned int total_population = 500;
	unsigned int initial_number_of_sick = 10; // Number of patients 0
	unsigned int incubation_period = 10; // Number of days before symptoms appear
	unsigned int is_infectious_since_day = 1; // On which incubation day the person becomes infectious
	unsigned int average_daily_interactions = 12; // Size of the daily interaction circle
	unsigned int hospital_capacity = 500; // Number of total available hospital beds

	incubating = new unsigned int[incubation_period];
	incubating[0] = initial_number_of_sick;
	for (int i = 1; i <= incubation_period; i++){
		incubating[i] = 0;
	}
	Population* archive[number_of_simulation_days+1];

	probability_of.getting_sick = 0.25;	// Chance of catching it from an infectious person they met
	probability_of.healthy_staying_home = 0.05; // Chance of prevention by self quarantine
	probability_of.mild_symptoms = 0.70; // Chance of developing mild symptoms after passing the incubation period (Leaving 30% chance to develop severe symptoms)
	probability_of.ms_staying_home = 0.25; // Chance of self quarantine after developing mild symptoms (Leaving 75% chance of staying in public)

	probability_of.hospital_recovery = 0.75;
	probability_of.hospital_death = 0.05;
	// Leaving 20% chance to stay in hospital for another day

	probability_of.home_recovery = 0.50;
	probability_of.home_death = 0.10;
	// Leaving 40% chance of needing hospitalization

	probability_of.post_recovery_paranoia = 0.45; // Chance of self quarantine after overcoming the illness

	Population population = Population(total_population, incubation_period, initial_number_of_sick,
									   is_infectious_since_day, average_daily_interactions, hospital_capacity);
	population.day = 1;

	while(number_of_simulation_days) {
		DEBUG(cout << "----- DAY " << population.day << " -----" << endl;);
		population.CalculateInteractions(local_debugging_enabled);

		/* Hospitals take action
		 * - Cure, lose or keep each patient for another day of treatment
		 * - Cured and lost patients free up beds
		 * - Admit people from ss_waiting_for_bed until the capacity is filled */
		population.Hospital(local_debugging_enabled);

		/* People in home self-quarantine are evaluated
		 * - Some die
		 * - Some recover and return to public
		 * - Some recognize their need for medical attention and are from the next day start waiting for a hospital bed */
		population.HomeQuarantine(local_debugging_enabled);

		/* After each day the incubating in incubation period advance to the next day.
		 * If they're past the incubation period, the next day they don't meet with
		 * anyone and either stay home or try to get admitted into the hospital to get treatment*/
		population.IllnessAdvances(local_debugging_enabled);

		population.Report();
		unsigned int c = population.day-1;
		Population temp = population;
		archive[c] = &temp;
		++population.day;
		--number_of_simulation_days;
	}

	archive[number_of_simulation_days]->Report();
	return 0;
}
