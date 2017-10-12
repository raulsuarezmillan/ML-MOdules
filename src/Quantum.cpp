#include "ML_modules.hpp"


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

#ifdef v032
	Quantum() ;
#endif

#ifdef v040
	Quantum() : Module( NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {};
#endif

	void step();
#ifdef v040
	PulseGenerator pulse;
#endif

	int last_octave=0, last_semi=0;

	bool semiState[12] = {};
	SchmittTrigger semiTriggers[12], setTrigger, resetTrigger;
	float semiLight[12] = {};

        void initialize() {
                for (int i = 0; i < 12; i++) {
                        semiState[i] = false;
			semiLight[i] = 0.0;
                }
		last_octave = 0;
		last_semi   = 0;
        }

	void randomize() {
		for (int i = 0; i<12; i++) {
			semiState[i] = (randomf() > 0.5);
			semiLight[i] = semiState[i]?1.0:0.0;
		};
		last_octave = 0;
		last_semi   = 0;

	}

        json_t *toJson() {
                json_t *rootJ = json_object();

                json_t *scaleJ = json_array();
                for (int i = 0; i < 12; i++) {
                        json_t *semiJ = json_integer( (int) semiState[i]);
                        json_array_append_new(scaleJ, semiJ);
                }
                json_object_set_new(rootJ, "scale", scaleJ);

                return rootJ;
        }

        void fromJson(json_t *rootJ) {
                json_t *scaleJ = json_object_get(rootJ, "scale");
                for (int i = 0; i < 12; i++) {
                        json_t *semiJ = json_array_get(scaleJ, i);
                        semiState[i] = !!json_integer_value(semiJ);
			semiLight[i] = semiState[i]?1.0:0.0;
                }
        }
	

};

#ifdef v032
Quantum::Quantum() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);
};
#endif


void Quantum::step() {

	for(int i=0; i<12; i++) {
#ifdef v032
		if (semiTriggers[i].process(params[Quantum::SEMI_1_PARAM + i])) {
#endif

#ifdef v040
		if (semiTriggers[i].process(params[Quantum::SEMI_1_PARAM + i].value)) {
#endif
                        semiState[i] = !semiState[i];
                }
		semiLight[i] = semiState[i]?1.0:0.0;

	}

	float gate = 0, trigger=0;
	float quantized;

#ifdef v032
	float v=getf(inputs[IN_INPUT]);
	float t=getf(inputs[TRANSPOSE_INPUT]);
	float n=getf(inputs[NOTE_INPUT]);
#endif

#ifdef v040
	float v=inputs[IN_INPUT].value;
	float t=inputs[TRANSPOSE_INPUT].normalize(0.0);
	float n=inputs[NOTE_INPUT].value;
#endif




	int octave   = round(v);
	int octave_t = round(t);
	int octave_n = round(n);

	int semi   = round( 12.0*(v - 1.0*octave) );
	int semi_t = round( 12.0*(t - 1.0*octave_t) );
	int semi_n = round( 12.0*(n - 1.0*octave_n) ) - semi_t;



	int tmp_semi=(semi-semi_t)%12;
	if(tmp_semi<0) tmp_semi+=12;
	if(semi_n<0) semi_n+=12;

#ifdef v032
       	if( inputs[RESET_INPUT] ) {
                if( resetTrigger.process(*inputs[RESET_INPUT]) ) initialize();
        };
#endif

#ifdef v040
       	if( inputs[RESET_INPUT].active ) {
                if( resetTrigger.process(inputs[RESET_INPUT].value) ) initialize();
        };
#endif

#ifdef v032
	if( inputs[SET_INPUT] ) {
		if( setTrigger.process(*inputs[SET_INPUT] ) ) {
			semiState[semi_n] = !semiState[semi_n];
			semiLight[semi_n] = semiState[semi_n]?1.0:0.0;
		}
	};
#endif

#ifdef v040
	if( inputs[SET_INPUT].active ) {
		if( setTrigger.process(inputs[SET_INPUT].value ) ) {
			semiState[semi_n] = !semiState[semi_n];
			semiLight[semi_n] = semiState[semi_n]?1.0:0.0;
		}
	};
#endif


	if( semiState[tmp_semi] ) 
	{ 
		bool changed = !( (octave==last_octave)&&(semi==last_semi));
		gate = 10.0; 
		quantized = 1.0*octave + semi/12.0;
#ifdef v032
		if(changed) trigger=10.0;
#endif

#ifdef v040
		if(changed) pulse.trigger(0.001);
#endif
		last_octave = octave;
		last_semi   = semi;

	} else {
		quantized = 1.0*last_octave + last_semi/12.0;
	};

#ifdef v040
	trigger = pulse.process(1.0/gSampleRate) ? 10.0 : 0.0;
#endif

#ifdef v032
	setf(outputs[OUT_OUTPUT], quantized);
	setf(outputs[GATE_OUTPUT], gate);
	setf(outputs[TRIGGER_OUTPUT], trigger);
#endif

#ifdef v040
	outputs[OUT_OUTPUT].value = quantized;
	outputs[GATE_OUTPUT].value = gate;
	outputs[TRIGGER_OUTPUT].value = trigger;
#endif

}



QuantumWidget::QuantumWidget() {
	Quantum *module = new Quantum();
	setModule(module);
	box.size = Vec(15*8, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
#ifdef v032
		panel->setBackground(SVG::load("plugins/ML_modules/res/Quantum.svg"));
#endif
#ifdef v040
		panel->setBackground(SVG::load(assetPlugin(plugin,"res/Quantum.svg")));
#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));

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
		addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(offset_x+5, -22*i+5+offset_y), &module->semiLight[Quantum::SEMI_1_PARAM+i]));

	}

}
