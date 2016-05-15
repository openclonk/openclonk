/*--
		Effect.c
		Authors: Günther

		Prototype for effect prototypes.
--*/

static const Effect = new Global {
	Remove = func(bool no_calls) {
		return RemoveEffect(nil, nil, this, no_calls);
	},
	// These properties are set on all effects by the engine.
	// They are declared here so that functions on proplists inheriting from this one can use them easily.
	Name = nil,
	Priority = 0,
	Interval = 0,
	Target = nil,
	Time = 0
};
