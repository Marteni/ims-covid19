/**********************************************
 *                 IMS Project                *
 *                                            *
 *   Epidemiological macro model simulation   *
 *         Martin Škorupa  (xskoru00)         *
 *          Diana Barnová  (xbarno00)         *
 **********************************************/

#include <iostream>
#include <random>
#include <getopt.h>
#include <fstream>

using namespace std;

bool debugging_enabled = false;
#define DEBUG(msg) do { \
  if (debugging_enabled) { msg } \
} while (0)

unsigned int* incubating;

struct probabilities_t {
	float getting_sick = 0.0;
	float healthy_staying_home = 0.0;
	float mild_symptoms = 0.0;
	float ms_staying_home = 0.0;

	float hospital_recovery = 0.0;
	float hospital_death = 0.0;
	float home_recovery = 0.0;

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

	/* Simulates the spread of infection between people in public
	 * - Gets all the people moving around the public and randomly composes groups simulating encounters
	 * - If an infectious person is in the group all the healthy people have a chance to catch the disease */
	void CalculateInteractions(bool local_debug_out_enabled = false) {
		local_debug_out_enabled ? debugging_enabled = true : debugging_enabled = false;

		unsigned int available, available_infectious, available_mildly_infectious, present_infectious, present_healthy, picked_person, x;
		available = available_infectious = available_mildly_infectious = present_infectious = present_healthy = picked_person = x = 0;

		DEBUG(cout << "Infection spreading events: " << endl;);
		// Get people that are infectious, but don't know it yet (still within incubation period)
		for (unsigned int i = this->is_infectious_since_day; i <= this->incubation_period; i++){
			available_infectious += incubating[i];
			DEBUG(cout << "I|  Incubated for " << i+1 << " days: " << incubating[i] << endl;);
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
							DEBUG(cout << "I|   - Became asymptomatic";);
							if (percentageFraction() <= probability_of.healthy_staying_home) {
								--this->healthy_in_public;
								++this->asymptomatic_at_home;
								DEBUG(cout << " and is going home" << endl;);
							}
							else{
								--this->healthy_in_public;
								++this->asymptomatic_in_public;
								++incubating[0];
								DEBUG(cout << " and is staying in public and incubating" << endl;);
							}
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
				DEBUG(cout << "I| -- Unevaluated people left: " << available << " of which healthy: " << available - (available_infectious + available_mildly_infectious) + 1 << " --" << endl;);
			}
			else { available = 0; }
			++x;
		}

		DEBUG(cout << " \\----------------" << endl;);
		local_debug_out_enabled ? debugging_enabled = false : debugging_enabled = true;
	}

	/* Hospitals take action
	 * - Cure, lose or keep each patient for another day of treatment
	 * - Cured and lost patients free up beds
	 * - Admit people from ss_waiting_for_bed until the capacity is filled */
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

		DEBUG(cout << " \\----------------" << endl;);
		local_debug_out_enabled ? debugging_enabled = false : debugging_enabled = true;
	}

	/* People in self-quarantine are evaluated
	 * - Some die
	 * - Some recover and return to public
	 * - Some recognize their need for medical attention and are from the next day start waiting for a hospital bed */
	void HomeQuarantine(bool local_debug_out_enabled = false){
		local_debug_out_enabled ? debugging_enabled = true : debugging_enabled = false;

		DEBUG(cout << "Home self quarantine events: " << endl;);
		for(unsigned int i = 1; i <= this->ms_at_home; i++){
			float fate = percentageFraction();

			// Person recovers at home an returns into public
			if(fate <= probability_of.home_recovery){
				--this->ms_at_home;
				++this->healthy_in_public;
				DEBUG(cout << "Q| - Mildly symptomatic " << i << " has recovered and returns to public." << endl;);
			}
				// Person's status has worsened and needs medical attention
			else{
				--this->ms_at_home;
				++this->ss_waiting_for_bed;
				DEBUG(cout << "Q| - Mildly symptomatic " << i
							<< " needs medical attention and is now waiting for a hospital bed." << endl;);
			}
		}
		for (unsigned int i = 1; i <= this->asymptomatic_at_home; ++i) {
			float fate = percentageFraction();

			// Person recovers at home an returns into public
			if (fate <= probability_of.home_recovery) {
				--this->asymptomatic_at_home;
				++this->healthy_in_public;
				DEBUG(cout << "Q| - Asymptomatic " << i << " has recovered and returns to public." << endl;);
			}
			// Person's status has worsened and needs medical attention
			else {
				--this->asymptomatic_at_home;
				++this->ss_waiting_for_bed;
				DEBUG(cout << "Q| - Asymptomatic " << i
						   << " needs medical attention and is now waiting for a hospital bed." << endl;);
			}
		}

		DEBUG(cout << " \\----------------" << endl;);
		local_debug_out_enabled ? debugging_enabled = false : debugging_enabled = true;
	}

	/* After each day the incubating in incubation period advance to the next day.
	 * If they're past the incubation period, the next day they don't meet with
	 * anyone and either stay home or try to get admitted into the hospital to get treatment */
	void IllnessAdvances(bool local_debug_out_enabled = false){
		local_debug_out_enabled ? debugging_enabled = true : debugging_enabled = false;

		unsigned int past_incubation_period = incubating[this->incubation_period];

		DEBUG(cout << "Illness advancing events: " << endl;);

		// Possibly advance mildly symptomatic people in public to severely symptomatic and send them to hospital
		unsigned int mildly_symptomatic = this->ms_in_public;
		this->ms_in_public = 0;
		DEBUG(cout << "A| Mildly symptomatic for reevaluation: " << mildly_symptomatic << endl;);
		while (mildly_symptomatic){
			if(percentageFraction() <= probability_of.mild_symptoms){ // Gain mild symptoms
				DEBUG(cout << "A|  Got mild symptoms - At home/In public " << this->ms_at_home << "/" << this->ms_in_public << " => ";);
				(percentageFraction() <= probability_of.ms_staying_home) ? ++this->ms_at_home : ++this->ms_in_public;
				DEBUG(cout << this->ms_at_home << "/" << this->ms_in_public << endl;);
			}
			else { // Gain severe symptoms that require hospitalization
				DEBUG(cout << "A|  Got severe symptoms and is now waiting for a hospital bed." << endl;);
				++this->ss_waiting_for_bed;
			}

			--mildly_symptomatic;
		}

		// Advance asymptotic incubating one day forward
		DEBUG(cout << "A| Incubating:"<< endl;);
		DEBUG(cout << "A|  | "; for (unsigned int a = 0; a <= this->incubation_period; ++a) { cout << incubating[a] << " | "; } cout << endl;);
		for(unsigned int i = this->incubation_period; i >= 1; --i){
			incubating[i] = incubating[i - 1];
			DEBUG(cout << "A|  | "; for (unsigned int a = 0; a <= this->incubation_period; ++a) { cout << incubating[a] << " | "; } cout << endl;);
		}
		incubating[0] = 0;
		DEBUG(cout << "A|  | "; for (unsigned int a = 0; a <= this->incubation_period; ++a) { cout << incubating[a] << " | "; } cout << endl;);

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

		DEBUG(cout << " \\----------------" << endl;);
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

void PrintHelp(){
	cout << "========== Help message for infection progress simulator ==========" << endl
		 << " Arguments:" << endl
		 << "   - simDays              Simulation length in days" << endl
		 << "   - population           Total population" << endl
		 << "   - initSick             Number of sick people on the first simulation day" << endl
		 << "   - incubPeriod          Incubation period (after which people develop mild or severe symptoms)" << endl
		 << "   - infectSince          Number of incubation days passed for the person to be infectious" << endl
		 << "   - avgDailyInter        Number of random people that meet every day" << endl
		 << "   - hospCap              Total number of hospital beds available" << endl
		 << "   - CgetSick             Chance of getting infected after meeting an infectious person (in %)" << endl
		 << "   - ChealthyAtHome       Chance of staying at home as a precaution after meeting an infectious person (in %)" << endl
		 << "   - CmildSympt           Chance of developing mild symptoms after the incubation period (in %)" << endl
		 << "   - CmildSymAtHome       Chance of staying at home when having mild symptoms (in %)" << endl
		 << "   - ChospitalRec         Chance of recovery when hospitalized (in %)" << endl
		 << "   - ChospitalDeath       Chance of dying when hospitalized (in %)" << endl
		 << "   - ChomeRec             Chance of recovering in home isolation (in %)" << endl
		 << "   - Cprp                 Chance of post recovery paranoia (staying at home until the end of the simulation) (in %)" << endl
		 << "  When an argument is not used it is All arguments have to have a whole positive number as a value." << endl
		 << endl
		 << " Chances deduced from chance arguments:" << endl
	  	 << "   * 100% - CgetSick%" << endl
	  	 << "	          -> Chance of staying healthy after meeting an infectious person" << endl
	  	 << "   * 100% - ChealthyAtHome%" << endl
	  	 << "	          -> Chance of staying in public after meeting an infectious person" << endl
	  	 << "   * 100% - CmildSympt%" << endl
	  	 << "	          -> Chance of developing severe symptoms after the incubation periodand going to the hospital" << endl
	  	 << "   * 100% - CmildSymAtHome%" << endl
	  	 << "	          -> Chance of staying in public after developing mild symptoms" << endl
	  	 << "   * 100% - ChospitalRec% - ChospitalDeath%" << endl
	  	 << "	          -> Chance of staying healthy after meeting an infectious person" << endl
	  	 << "   * 100% - ChomeRec%" << endl
	  	 << "	          -> Chance of developing severe symptoms at home and going to the hospital" << endl
	  	 << "   * 100% - Cprp%" << endl
	  	 << "	          -> Chance of staying in public after recovering" << endl
	  	 << endl;
}

int main(int argc, char* argv[]) {
	bool local_debugging_enabled = false;

	unsigned int number_of_simulation_days = 7;

	unsigned int total_population = 1324277;
	unsigned int initial_number_of_sick = 6834; // Number of patients 0
	unsigned int incubation_period = 5; // Number of days before symptoms appear
	unsigned int is_infectious_since_day = 4; // On which incubation day the person becomes infectious
	unsigned int average_daily_interactions = 2000; // Size of the daily interaction circle
	unsigned int hospital_capacity = 836; // Number of total available hospital beds


	probability_of.getting_sick = 0.10;	// Chance of catching it from an infectious person they met
	probability_of.healthy_staying_home = 0.05; // Chance of prevention by self quarantine
	probability_of.mild_symptoms = 0.80; // Chance of developing mild symptoms after passing the incubation period (Leaving 20% chance to develop severe symptoms)
	probability_of.ms_staying_home = 0.95; // Chance of self quarantine after developing mild symptoms (Leaving 5% chance of staying in public)

	probability_of.hospital_recovery = 0.90;
	probability_of.hospital_death = 0.03;
	// Leaving 7% chance to stay in hospital for another day

	probability_of.home_recovery = 0.90;
	// Leaving 60% chance of needing hospitalization

	probability_of.post_recovery_paranoia = 0.15;
	// Leaving 85% chance of self quarantine after overcoming the illness

	const option long_opts[] = {
			{"simDays", required_argument, nullptr, 'a'},
			{"population", required_argument, nullptr, 'b'},
			{"initSick", required_argument, nullptr, 'c'},
			{"incubPeriod", required_argument, nullptr, 'd'},
			{"infectSince", required_argument, nullptr, 'e'},
			{"avgDailyInter", required_argument, nullptr, 'f'},
			{"hospCap", required_argument, nullptr, 'g'},
			{"CgetSick", required_argument, nullptr, 'q'},
			{"ChealthyAtHome", required_argument, nullptr, 'i'},
			{"CmildSympt", required_argument, nullptr, 'j'},
			{"CmildSymAtHome", required_argument, nullptr, 'k'},
			{"ChospitalRec", required_argument, nullptr, 'l'},
			{"ChospitalDeath", required_argument, nullptr, 'm'},
			{"ChomeRec", required_argument, nullptr, 'n'},
			{"Cprp", required_argument, nullptr, 'p'},
			{"help", no_argument, nullptr, 'h'},
			{nullptr, no_argument, nullptr, 0}
	};

	local_debugging_enabled ? debugging_enabled = true : debugging_enabled = false;
	while (true)
	{
		const auto opt = getopt_long_only(argc, argv, "", long_opts, nullptr);

		if (opt == -1)
			break;

		switch (opt)
		{
			case 'a':
				number_of_simulation_days = std::stoul(optarg);
				DEBUG(std::cout << "Number of simulation days set to: " << number_of_simulation_days << endl;);
				break;
			case 'b':
				total_population = std::stoul(optarg);
				DEBUG(std::cout << "Total population set to: " << total_population << endl;);
				break;
			case 'c':
				initial_number_of_sick = std::stoul(optarg);
				DEBUG(std::cout << "Initial number of sick set to: " << initial_number_of_sick << std::endl;);
				break;
			case 'd':
				incubation_period = std::stoul(optarg);
				DEBUG(std::cout << "Incubation period set to: " << incubation_period << std::endl;);
				break;
			case 'e':
				is_infectious_since_day = std::stoul(optarg);
				DEBUG(std::cout << "Infectious since day X set to: " << is_infectious_since_day << std::endl;);
				break;
			case 'f':
				average_daily_interactions = std::stoul(optarg);
				DEBUG(std::cout << "Average daily interactions set to: " << average_daily_interactions << std::endl;);
				break;
			case 'g':
				hospital_capacity = std::stoul(optarg);
				DEBUG(std::cout << "Hospital bed capacity set to: " << hospital_capacity << std::endl;);
				break;
			case 'q':
				probability_of.getting_sick = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of getting sick set to: " << probability_of.getting_sick*100 << "%" << std::endl;);
				break;
			case 'i':
				probability_of.healthy_staying_home = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of healthy people isolating set to: " << probability_of.healthy_staying_home*100 << "%" << std::endl;);
				break;
			case 'j':
				probability_of.mild_symptoms = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of developing mild symptoms set to: " << probability_of.mild_symptoms*100 << "%" << std::endl;);
				break;
			case 'k':
				probability_of.ms_staying_home = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of staying home when having mild symptoms set to: " << probability_of.ms_staying_home*100 << "%" << std::endl;);
				break;
			case 'l':
				probability_of.hospital_recovery = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of recovery when hospitalized set to: " << probability_of.hospital_recovery*100 << "%" << std::endl;);
				break;
			case 'm':
				probability_of.hospital_death = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of dying when hospitalized set to: " << probability_of.hospital_death*100 << "%" << std::endl;);
				break;
			case 'n':
				probability_of.home_recovery = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of recovering in home isolation set to: " << probability_of.home_recovery*100 << "%" << std::endl;);
				break;
			case 'p':
				probability_of.post_recovery_paranoia = (float)(std::stoi(optarg))/100;
				DEBUG(std::cout << "Probability of post recovery paranoia set to: " << probability_of.post_recovery_paranoia*100 << "%" << std::endl;);
				break;
			case 'h': // -h or --help
			case '?': // Unrecognized option
			default:
				PrintHelp();
				return 0;
		}	
	}
	local_debugging_enabled ? debugging_enabled = false : debugging_enabled = true;

	incubating = new unsigned int[incubation_period];
	incubating[0] = initial_number_of_sick;
	for (unsigned int i = 1; i < incubation_period; i++){
		incubating[i] = 0;
	}
	const unsigned int n_of_sim_days = number_of_simulation_days;
	Population* archive[n_of_sim_days];

	Population population = Population(total_population, incubation_period, initial_number_of_sick,
									   is_infectious_since_day, average_daily_interactions, hospital_capacity);
	population.day = 0;

	while(number_of_simulation_days) {
		++population.day;
		debugging_enabled = local_debugging_enabled;
		DEBUG(cout << "----- DAY " << population.day << " -----" << endl;);

		population.CalculateInteractions(local_debugging_enabled);

		population.HomeQuarantine(local_debugging_enabled);

		population.IllnessAdvances(local_debugging_enabled);

		population.Hospital(local_debugging_enabled);

		debugging_enabled = local_debugging_enabled;
		DEBUG(population.Report(););
		unsigned int c = population.day-1;
		archive[c] = new Population(0,0,0,0,0,0);
		*archive[c] = population;
		--number_of_simulation_days;
	}

	ofstream myfile ("data.dat");
	if (myfile.is_open())
	{
		myfile << "# Day Sick Dead Healthy Asymptomatic Mildly_symptomatic Severely_symptomatic\n";
		for(unsigned int i = 0; i < population.day; ++i){
			myfile << archive[i]->day
					<< " " << archive[i]->total_population - archive[i]->dead - (archive[i]->healthy_at_home + archive[i]->healthy_in_public)
					<< " " << archive[i]->dead
				  	<< " " << archive[i]->healthy_at_home + archive[i]->healthy_in_public
					<< " " << archive[i]->asymptomatic_at_home + archive[i]->asymptomatic_in_public
					<< " " << archive[i]->ms_at_home + archive[i]->ms_in_public
					<< " " << archive[i]->ss_waiting_for_bed + archive[i]->ss_in_bed << "\n";
		}
		myfile.close();
	}
	else cout << "Unable to open file";

	population.Report();
	return 0;
}
