(
~faustTests = (
	allTests: [
		\testBasic,
		\testNoScript,
		\testParameter,
	],

	testBasic: {
		var success = false;
		var condition = Condition();
		FaustDef(\testBasic, "process = 0.5;").send;
		s.sync;
		{
			Faust.ar(1, \testBasic);
		}.loadToFloatArray(0.1, action: {|sig|
			success = (sig[0]-0.5).abs <= 0.01;
			condition.unhang;
		});
		condition.hang;
		success;
	},

	testNoScript: {
		var success = false;
		// an unregistered script fails on a language level
		try {
			{Faust.ar(1, \noScript)}.loadToFloatArray(0.1);
		} {|error|
			success = true;
		};
		success;
	},

	testParameter: {
		var success = false;
		var condition = Condition();
		FaustDef(\testParam, "process = hslider(\"foo\",0,0,1,0.1);").send;
		s.sync;
		{
			Faust.ar(1, \testParam, params: [\foo, DC.ar(pi)]);
		}.loadToFloatArray(0.1, action: {|sig|
			success = (sig[0]-pi).abs <= 0.01;
			condition.unhang;
		});
		condition.hang;
		success;
	},

	// meta

	run: {|self, name=nil|
		var testsToRun = if(name.isNil, {self.allTests}, {name.asArray});
		var results = testsToRun.collect({|testName|
			"Running %".format(testName).postln;
			[testName, self.perform(testName)];
		});
		if(results.any(_[1].not), {
			"ERROR: There were errors".postln;
			results.select(_[1].not).do({|test|
				"ERROR: Test % failed!".format(test[0]).postln;
			});
		}, {
			"All tests ran successfully!".postln;
		});

	},
)
)

s.reboot;

fork{~faustTests.run};
