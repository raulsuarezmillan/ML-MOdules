#include "ML_modules.hpp"

#include "dsp/digital.hpp"

struct Quantum : Module {
	enum ParamIds {
		SEMI_1_PARAM,
		SEMI_2_PARAM,
		SEMI_3_PARAM,
		SEMI_4_PARAM,
		SEMI_5_PARAM,
		SEMI_6_PARAM,
		SEMI_7_PARAM,
		SEMI_8_PARAM,
		SEMI_9_PARAM,
		SEMI_10_PARAM,
		SEMI_11_PARAM,
		SEMI_12_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN_INPUT,
		TRANSPOSE_INPUT,
		NOTE_INPUT,
		SET_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		TRIGGER_OUTPUT,
		GATE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		SEMI_1_LIGHT,
		SEMI_2_LIGHT,
		SEMI_3_LIGHT,
		SEMI_4_LIGHT,
		SEMI_5_LIGHT,
		SEMI_6_LIGHT,
		SEMI_7_LIGHT,
		SEMI_8_LIGHT,
		SEMI_9_LIGHT,
		SEMI_10_LIGHT,
		SEMI_11_LIGHT,
		SEMI_12_LIGHT,
		NUM_LIGHTS
	};

	enum Mode {
		LAST,
		CLOSEST_UP,
		CLOSEST_DOWN,
		UP,
		DOWN
	};

	Mode mode = LAST;
		

	Quantum() : Module( NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {};

	void step() override;
	PulseGenerator pulse;

	int last_octave=0, last_semi=0;

	bool semiState[12] = {};
	SchmittTrigger semiTriggers[12], setTrigger, resetTrigger;
	float semiLight[12] = {};

#ifdef v_dev
        void reset() override {
#endif
#ifdef v040
        void initialize() override {
#endif
                for (int i = 0; i < 12; i++) {
                        semiState[i] = false;
			semiLight[i] = 0.0;
                }
		last_octave = 0;
		last_semi   = 0;
        }

	void randomize() override {
		for (int i = 0; i<12; i++) {
			semiState[i] = (randomf() > 0.5);
			semiLight[i] = semiState[i]?1.0:0.0;
		};
		last_octave = 0;
		last_semi   = 0;

	}

        json_t *toJson() override {
                json_t *rootJ = json_object();

                json_t *scaleJ = json_array();
                for (int i = 0; i < 12; i++) {
                        json_t *semiJ = json_integer( (int) semiState[i]);
                        json_array_append_new(scaleJ, semiJ);
                }
                json_object_set_new(rootJ, "scale", scaleJ);

                return rootJ;
        }

        void fromJson(json_t *rootJ) override {
                json_t *scaleJ = json_object_get(rootJ, "scale");
                for (int i = 0; i < 12; i++) {
                        json_t *semiJ = json_array_get(scaleJ, i);
                        semiState[i] = !!json_integer_value(semiJ);
			semiLight[i] = semiState[i]?1.0:0.0;
                }
        }

};


void Quantum::step() {

	for(int i=0; i<12; i++) {

		if (semiTriggers[i].process(params[Quantum::SEMI_1_PARAM + i].value)) {
                        semiState[i] = !semiState[i];
                }
#ifdef v040
		semiLight[i] = semiState[i]?1.0:0.0;
#endif

#ifdef v_dev
		lights[i].value = semiState[i]?1.0:0.0;
#endif
	}

	float gate = 0, trigger=0;
	float quantized = 0;

	float v=inputs[IN_INPUT].value;
	float t=inputs[TRANSPOSE_INPUT].normalize(0.0);
	float n=inputs[NOTE_INPUT].value;

	int octave   = round(v);
	int octave_t = round(t);
	int octave_n = round(n);

	int semi   = round( 12.0*(v - 1.0*octave) );
	int semi_t = round( 12.0*(t - 1.0*octave_t) );
	int semi_n = round( 12.0*(n - 1.0*octave_n) ) - semi_t;


	// transpose to shifted scale:

	int tmp_semi=(semi-semi_t)%12;

	if(tmp_semi<0) tmp_semi+=12;
	if(semi_n<0) semi_n+=12;


       	if( inputs[RESET_INPUT].active ) {
#ifdef v040
                if( resetTrigger.process(inputs[RESET_INPUT].value) ) initialize();
#endif
#ifdef v_dev
                if( resetTrigger.process(inputs[RESET_INPUT].value) ) reset();
#endif
        };


	if( inputs[SET_INPUT].active ) {
		if( setTrigger.process(inputs[SET_INPUT].value ) ) {
			semiState[semi_n] = !semiState[semi_n];
			semiLight[semi_n] = semiState[semi_n]?1.0:0.0;
		}
	};


	if( semiState[tmp_semi] ) 
	{ 
		bool changed = !( (octave==last_octave)&&(semi==last_semi));
		gate = 10.0; 
		quantized = 1.0*octave + semi/12.0;
		if(changed) pulse.trigger(0.001);
		last_octave = octave;
		last_semi   = semi;

	} else {

		if(mode==LAST) {
			quantized = 1.0*last_octave + last_semi/12.0;
		} else {

			int i_up   = 0;
			int i_down = 0;

			while( !semiState[tmp_semi+i_up  ] ) i_up++;
		 	while( !semiState[tmp_semi-i_down] ) i_down++;
	
			switch(mode) {	
			case UP: 		semi = semi+i_up; break;
			case DOWN: 		semi = semi-i_down; break;
			case CLOSEST_UP:   	semi = (i_up > i_down)? semi - i_down : semi + i_up; break;
			case CLOSEST_DOWN: 	semi = (i_down > i_up)? semi + i_up : semi - i_down; break;
			case LAST:	
			default:		break;
			};

			bool changed = !( (octave==last_octave)&&(semi==last_semi));
			quantized = 1.0*octave + semi/12.0;
			if(changed) pulse.trigger(0.001);
			last_octave = octave;
			last_semi   = semi;
		};
	};

#ifdef v_dev
	float gSampleRate = engineGetSampleRate();
#endif

	trigger = pulse.process(1.0/gSampleRate) ? 10.0 : 0.0;


	outputs[OUT_OUTPUT].value = quantized;
	outputs[GATE_OUTPUT].value = gate;
	outputs[TRIGGER_OUTPUT].value = trigger;

}

struct QuantumModeItem : MenuItem {

        Quantum *quantum;
        Quantum::Mode mode;

#ifdef v040
        void onAction() override {
                quantum->mode = mode;
        };
#endif

#ifdef v_dev
        void onAction(EventAction &e) override {
                quantum->mode = mode;
        };
#endif

        void step() override {
                rightText = (quantum->mode == mode)? "✔" : "";
        };

};

Menu *QuantumWidget::createContextMenu() {

        Menu *menu = ModuleWidget::createContextMenu();

        MenuLabel *spacerLabel = new MenuLabel();
        menu->pushChild(spacerLabel);

        Quantum *quantum = dynamic_cast<Quantum*>(module);
        assert(quantum);

        MenuLabel *modeLabel = new MenuLabel();
        modeLabel->text = "Mode";
        menu->pushChild(modeLabel);

        QuantumModeItem *last_Item = new QuantumModeItem();
        last_Item->text = "Last";
        last_Item->quantum = quantum;
        last_Item->mode = Quantum::LAST;
        menu->pushChild(last_Item);

        QuantumModeItem *up_Item = new QuantumModeItem();
        up_Item->text = "Up";
        up_Item->quantum = quantum;
        up_Item->mode = Quantum::UP;
        menu->pushChild(up_Item);

        QuantumModeItem *down_Item = new QuantumModeItem();
        down_Item->text = "Down";
        down_Item->quantum = quantum;
        down_Item->mode = Quantum::DOWN;
        menu->pushChild(down_Item);

        QuantumModeItem *cl_up_Item = new QuantumModeItem();
        cl_up_Item->text = "Closest, up";
        cl_up_Item->quantum = quantum;
        cl_up_Item->mode = Quantum::CLOSEST_UP;
        menu->pushChild(cl_up_Item);

        QuantumModeItem *cl_dn_Item = new QuantumModeItem();
        cl_dn_Item->text = "Closest, Down";
        cl_dn_Item->quantum = quantum;
        cl_dn_Item->mode = Quantum::CLOSEST_DOWN;
        menu->pushChild(cl_dn_Item);

	return menu;
};

QuantumWidget::QuantumWidget() {
	Quantum *module = new Quantum();
	setModule(module);
	box.size = Vec(15*8, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin,"res/Quantum.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addInput(createInput<PJ301MPort>(Vec(19, 42), module, Quantum::IN_INPUT));
	addOutput(createOutput<PJ301MPort>(Vec(76, 42), module, Quantum::OUT_OUTPUT));

	addInput(createInput<PJ301MPort>(Vec(76, 90), module, Quantum::TRANSPOSE_INPUT));
	addOutput(createOutput<PJ301MPort>(Vec(76, 140), module, Quantum::GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(76, 180), module, Quantum::TRIGGER_OUTPUT));

	addInput(createInput<PJ301MPort>(Vec(76, 226), module, Quantum::NOTE_INPUT));
	addInput(createInput<PJ301MPort>(Vec(76, 266), module, Quantum::SET_INPUT));
	addInput(createInput<PJ301MPort>(Vec(76, 312), module, Quantum::RESET_INPUT));

	static const float offset_x = 23;
	static const float offset_y = 332;

	for(int i=0; i<12; i++) {
		addParam(createParam<LEDButton>(Vec(offset_x, -22*i+offset_y), module, Quantum::SEMI_1_PARAM + i, 0.0, 1.0, 0.0));
#ifdef v040
		addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(offset_x+5, -22*i+5+offset_y), &module->semiLight[Quantum::SEMI_1_PARAM+i]));
#endif
#ifdef v_dev
		addChild(createLight<SmallLight<GreenLight>>(Vec(offset_x+5, -22*i+5+offset_y), module, Quantum::SEMI_1_LIGHT+i));
#endif
	}

}
