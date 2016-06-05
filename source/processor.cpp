#include <iostream>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TBox.h"
#include "TAxis.h"
#include "TROOT.h"
#include "TFrame.h"

#define YMIN 20
#define YMAX 100

bool processTree(std::vector<double> &vals1, std::vector<double> &vals2, TTree *t_){
	if(!t_){ return false; }
	
	double r1;
	TBranch *b1;
	t_->SetBranchAddress("r1", &r1, &b1);
	
	double seconds;
	TBranch *bsec;
	t_->SetBranchAddress("seconds", &seconds, &bsec);
	
	if(!b1 || !bsec){ return false; }
	
	std::cout << " Processing " << t_->GetEntries() << " entries.\n";
	
	t_->GetEntry(0);
	bool prev_r1_state = false;
	if(r1 == 1){
		vals1.push_back(seconds);
		std::cout << seconds;
		prev_r1_state = true;
	}
	
	for(int i = 1; i < t_->GetEntries(); i++){
		t_->GetEntry(i);
		if(!prev_r1_state && r1 == 1){
			vals1.push_back(seconds);
			std::cout << seconds;
			prev_r1_state = true;
		}
		else if(prev_r1_state && r1 == 0){
			vals2.push_back(seconds);
			std::cout << "\t" << seconds << std::endl;
			prev_r1_state = false;
		}
	}
	
	return (vals1.size() == vals2.size());
}

void processGraph1(const std::vector<double> &vals1, const std::vector<double> &vals2, TGraph *g_){
	if(!g_){ return; }

	TCanvas *can = new TCanvas("can1");
	can->cd();
	
	can->SetFrameLineWidth(0);
	g_->Draw("AL");
	g_->GetYaxis()->SetRangeUser(YMIN, YMAX);
	
	TBox *boxes[vals1.size()];
	
	for(size_t i = 0; i < vals1.size(); i++){
		boxes[i] = new TBox(vals1[i], YMIN, vals2[i], YMAX);
		boxes[i]->SetFillColor(9);
		boxes[i]->SetFillStyle(3002);
		boxes[i]->Draw("SAME");
	}
	can->Update();
	
	can->Print("temp.pdf");
	
	can->Close();
}

void processGraph2(TGraph *g_){
	if(!g_){ return; }

	TCanvas *can = new TCanvas("can2");
	((TPad*)can->cd())->SetLogy();
	
	can->SetFrameLineWidth(0);
	g_->Draw("AL");
	can->Update();
	
	can->Print("pres.pdf");
	
	can->Close();
}

void process(TFile *f1, TFile *f2){
	if(!f1 || !f2){ return; }
	
	TTree *tree = (TTree*)f1->Get("data");
	TGraph *graph1 = (TGraph*)f2->Get("temp");
	TGraph *graph2 = (TGraph*)f2->Get("pres");
	
	if(!tree || !graph1 || !graph2){ return; }
	
	graph1->SetTitle(0);
	graph1->GetXaxis()->SetTitle("Time (s)");
	graph1->GetYaxis()->SetTitle("Temperature (C)");

	graph2->SetTitle(0);
	graph2->GetXaxis()->SetTitle("Time (s)");
	graph2->GetYaxis()->SetTitle("Pressure (Torr)");
	
	std::vector<double> v1, v2;
	if(!processTree(v1, v2, tree)){ return; }
	
	processGraph1(v1, v2, graph1);
	processGraph2(graph2);
}

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <dataFile> <graphFile>\n";
}

int main(int argc, char* argv[]){
	if(argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)){
		help(argv[0]);
		return 0;
	}
	else if(argc < 3){
		std::cout << " Error: Invalid number of arguments to " << argv[0] << ". Expected 2, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}
	
	gROOT->SetBatch(1);
	
	TFile *dfile = new TFile(argv[1], "READ");
	if(!dfile->IsOpen()){
		std::cout << " ERROR! Failed to open input data file '" << argv[1] << "'!\n";
		return 1;
	}
	
	TFile *gfile = new TFile(argv[2], "READ");
	if(!gfile->IsOpen()){
		std::cout << " ERROR! Failed to open input graph file '" << argv[2] << "'!\n";
		dfile->Close();
		return 1;
	}
	
	process(dfile, gfile);
	dfile->Close();
	gfile->Close();
		
	return 0;
}
