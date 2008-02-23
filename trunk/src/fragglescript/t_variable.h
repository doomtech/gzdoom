
enum
{
	VARIABLESLOTS = 16
};

     // svariable_t
struct svariable_t
{
	FString Name;
	svariable_t *next;       // for hashing

//private:
	int type;       // vt_string or vt_int: same as in svalue_t
	FString string;

	union value_t
	{
		SDWORD i;
		//AActor *mobj;
		DActorPointer * acp;
		fixed_t fixed;          // haleyjd: fixed-point
		
		char **pS;              // pointer to game string
		int *pI;                // pointer to game int
		AActor **pMobj;         // pointer to game obj
		fixed_t *pFixed;        // haleyjd: fixed ptr
		
		void (*handler)();      // for functions
		char *labelptr;         // for labels
	} value;

public:

	svariable_t(const char *_name);
	~svariable_t();

	int Type() const
	{
		return type;
	}

	void ChangeType(int newtype)
	{
		if (type==svt_mobj && newtype!=svt_mobj)
		{
			value.acp->Destroy();
		}
		else if (type!=svt_mobj && newtype==svt_mobj)
		{
			value.acp = new DActorPointer;
		}
		type = newtype;
	}

	const value_t & Value() const
	{
		return value;
	}

	svalue_t GetValue() const;
	void SetValue(const svalue_t &newvalue);
	void Serialize(FArchive &ar);
};

// hash the variables for speed: this is the hashkey

inline int variable_hash(const char *n)
{
	return 
              (n[0]? (   ( (n)[0] + (n)[1] +   
                   ((n)[1] ? (n)[2] +   
				   ((n)[2] ? (n)[3]  : 0) : 0) ) % VARIABLESLOTS ) :0);
}

