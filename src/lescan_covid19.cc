#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <cerrno>
#include <array>
#include <iomanip>
#include <vector>
#include <map>
#include <boost/optional.hpp>
#include <ctime> 
#include <signal.h>

#include <stdexcept>

#include <blepp/logging.h>
#include <blepp/pretty_printers.h>
#include <blepp/blestatemachine.h> 
#include <blepp/lescan.h>

#include <sys/time.h>

using namespace std;
using namespace BLEPP;

void catch_function(int)
{
	cerr << "\nInterrupted!\n";
}
void print_time() {
	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64], buf[64];

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S.", nowtm);
	cout << tmbuf;
	snprintf(buf, sizeof buf, "%06ld", tv.tv_usec);
	cout << buf;
}

int main(int argc, char** argv)
{
	HCIScanner::ScanType type = HCIScanner::ScanType::Active;
	HCIScanner::FilterDuplicates filter = HCIScanner::FilterDuplicates::Software;
	int c;
	string help = R"X(-[cah]:
  -c  print covid-19 packets
  -a  print all packets
  -t  type: active | passive
  -h  show this message
)X";


	bool print_all_packets = false;

	filter = HCIScanner::FilterDuplicates::Off;

	opterr = 0;

	vector<string> uuids;

	uuids.push_back("FD6F");

	while((c=getopt(argc, argv, "!cat:h")) != -1)
	{
		switch(c) {
		case 'c': {
			break;
		}
		case 'a': {
			print_all_packets = true;
			break;
		}
		case 'h': {
			cout << "Usage: " << argv[0] << " " << help;
			return 0;
		}
		case 't': {
			string stype = string(optarg);
			if (stype == "passive") {
				type = HCIScanner::ScanType::Passive;
			} else {
				type = HCIScanner::ScanType::Active;
			}
			break;
		}
		case '?':
			if (optopt == 't') {
				cerr << "Option t requires an argument (active, passive)" << endl;
			}
			return 1;
		default:
			cerr << argv[0] << ":  unknown option " << c << endl;
			return 1;
		}
	}
	if (uuids.size() == 0 && print_all_packets == false) {
		cerr << "Option u is required, e.g -u 1000 or option -a" << endl;
		return 1;
	}

	if (print_all_packets) {
		cout << "Print all packets" << endl;
	}

	switch(type) {
	case HCIScanner::ScanType::Passive:
		cout << "Passive scanning" << endl;
		break;
	case HCIScanner::ScanType::Active:
		cout << "Active scanning" << endl;
		break;
	}

	log_level = LogLevels::Error;
	HCIScanner *scanner;
	try {
		scanner = new HCIScanner(true, filter, type);
	} catch(BLEPP::HCIScanner::HCIError) {
		cerr << "Does the device actually exist?" << endl;
		cerr << "Try something like this (reload the kernel module, unblock via rfkill, bring up interface):" << endl;
		cerr << "  sudo rmmod btusb" << endl;
		cerr << "  sudo modprobe btusb" << endl;
		cerr << "  sudo rfkill unblock bluetooth" << endl;
		cerr << "  sudo hciconfig hci0 up" << endl;
		exit(1);
	} catch(BLEPP::HCIScanner::IOError) {
		cerr << "You probably can solve this by using sudo." << endl;
		cerr << "  sudo ./lescan_covid19 -c" << endl;
		cerr << "You can also give the binary these so-called \"capabilities\" to be able to scan (and run as normal user)" << endl;
		cerr << "  sudo setcap cap_net_raw+ep lescan_covid19" << endl;
		cerr << "  ./lescan_covid19 -c" << endl;
		exit(2);
	}

	//Catch the interrupt signal. If the scanner is not 
	//cleaned up properly, then it doesn't reset the HCI state.
	signal(SIGINT, catch_function);

	//Something to print to demonstrate the timeout.
	string throbber="/|\\-";

	//hide cursor, to make the throbber look nicer.
	cout << "[?25l" << flush;

	bool print_packet = false;

	int i=0;
	while (1) {

		//Check to see if there's anything to read from the HCI
		//and wait if there's not.
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 300000;

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(scanner->get_fd(), &fds);
		int err = select(scanner->get_fd()+1, &fds, NULL, NULL,  &timeout);

		//Interrupted, so quit and clean up properly.
		if(err < 0 && errno == EINTR) {
			break;
		}

		if(FD_ISSET(scanner->get_fd(), &fds))
		{
			//Only read id there's something to read
			vector<AdvertisingResponse> ads = scanner->get_advertisements();

			bool unknown = false;
			string type = "UNKNOWN";
			for(const auto& ad: ads)
			{
				// Note that the types are not having the values in the LeAdvertisingEventType enum as they do over the air
				// The over the air values are listed after it in binary and hex.
				switch(ad.type) {
				case LeAdvertisingEventType::ADV_IND:
					type = "UNDIRECTED"; // connectable undirected     0000b = 0x0
					break;
				case LeAdvertisingEventType::ADV_DIRECT_IND:
					type = "DIRECTED"; // connectable directed         0001b = 0x1
					break;
				case LeAdvertisingEventType::ADV_NONCONN_IND:
					type = "NONCONNECTABLE"; // non-connectable        0010b = 0x2
					break;
				case LeAdvertisingEventType::ADV_SCAN_IND:
					type = "SCANNABLE"; // scannable                   0110b = 0x6
					break;

					//	case LeAdvertisingEventType::SCAN_REQ:
					//	  type = "SCAN_REQ"; // scan_req                     0011b = 0x3
					//	  break;
				case LeAdvertisingEventType::SCAN_RSP:
					type = "SCAN_RSP"; // scan_rsp                     0100b = 0x4
					break;
				default:
					type = std::to_string((int)ad.type);
					unknown = true;
				}

				if (ad.manufacturer_specific_data.size() > 0) {
				} else {
					for (const auto& uuid: uuids) {
						for(const auto& found_uuid: ad.UUIDs) {
							if (found_uuid == uuid) {
								print_time();
								cout << " " << ad.address << " " << string(uuid) << " " << (int) ad.rssi << " dBm " << "PLAIN " << type << endl;
							}
						}
					}
				}
			}
		}
		else
			cout << throbber[i%4] << "\b" << flush;
		i++;
	}

	//show cursor
	cout << "[?25h" << flush;
}
