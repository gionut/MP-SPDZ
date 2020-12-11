
#ifndef _Processor
#define _Processor

/* This is a representation of a processing element
 */

#include "Math/Integer.h"
#include "Exceptions/Exceptions.h"
#include "Networking/Player.h"
#include "Data_Files.h"
#include "Input.h"
#include "PrivateOutput.h"
#include "ExternalClients.h"
#include "Binary_File_IO.h"
#include "Instruction.h"
#include "ProcessorBase.h"
#include "OnlineOptions.h"
#include "Tools/SwitchableOutput.h"
#include "Tools/CheckVector.h"
#include "GC/Processor.h"
#include "GC/ShareThread.h"

class Program;

template <class T>
class SubProcessor
{
  CheckVector<typename T::clear> C;
  CheckVector<T> S;

  DataPositions bit_usage;

  void resize(int size)       { C.resize(size); S.resize(size); }

  template<class sint, class sgf2n> friend class Processor;
  template<class U> friend class SPDZ;
  template<class U> friend class ProtocolBase;
  template<class U> friend class Beaver;

  typedef typename T::bit_type::part_type BT;

public:
  ArithmeticProcessor* Proc;
  typename T::MAC_Check& MC;
  Player& P;
  Preprocessing<T>& DataF;

  typename T::Protocol protocol;
  typename T::Input input;

  typename BT::LivePrep bit_prep;
  vector<typename BT::LivePrep*> personal_bit_preps;

  SubProcessor(ArithmeticProcessor& Proc, typename T::MAC_Check& MC,
      Preprocessing<T>& DataF, Player& P);
  SubProcessor(typename T::MAC_Check& MC, Preprocessing<T>& DataF, Player& P,
      ArithmeticProcessor* Proc = 0);
  ~SubProcessor();

  // Access to PO (via calls to POpen start/stop)
  void POpen(const vector<int>& reg,const Player& P,int size);

  void muls(const vector<int>& reg, int size);
  void mulrs(const vector<int>& reg);
  void dotprods(const vector<int>& reg, int size);
  void matmuls(const vector<T>& source, const Instruction& instruction, int a,
      int b);
  void matmulsm(const CheckVector<T>& source, const Instruction& instruction, int a,
      int b);
  void conv2ds(const Instruction& instruction);

  CheckVector<T>& get_S()
  {
    return S;
  }

  CheckVector<typename T::clear>& get_C()
  {
    return C;
  }

  T& get_S_ref(int i)
  {
    return S[i];
  }

  typename T::clear& get_C_ref(int i)
  {
    return C[i];
  }
};

class ArithmeticProcessor : public ProcessorBase
{
protected:
  CheckVector<long> Ci;

public:
  int thread_num;

  PRNG secure_prng;

  string private_input_filename;
  string public_input_filename;

  ifstream private_input;
  ifstream public_input;
  ofstream public_output;
  ofstream private_output;
  ofstream stdout_redirect_file;

  int sent, rounds;

  OnlineOptions opts;

  ArithmeticProcessor() :
      ArithmeticProcessor(OnlineOptions::singleton, BaseMachine::thread_num)
  {
  }
  ArithmeticProcessor(OnlineOptions opts, int thread_num) : thread_num(thread_num),
          sent(0), rounds(0), opts(opts) {}

  bool use_stdin()
  {
    return thread_num == 0 and opts.interactive;
  }

  const long& read_Ci(int i) const
    { return Ci[i]; }
  long& get_Ci_ref(int i)
    { return Ci[i]; }
  void write_Ci(int i,const long& x)
    { Ci[i]=x; }
  CheckVector<long>& get_Ci()
    { return Ci; }
};

template<class sint, class sgf2n>
class Processor : public ArithmeticProcessor
{
  int reg_max2, reg_maxi;

  // Data structure used for reading/writing data to/from a socket (i.e. an external party to SPDZ)
  octetStream socket_stream;

  public:
  Data_Files<sint, sgf2n> DataF;
  Player& P;
  typename sgf2n::MAC_Check& MC2;
  typename sint::MAC_Check& MCp;
  Machine<sint, sgf2n>& machine;

  GC::ShareThread<typename sint::bit_type> share_thread;
  GC::Processor<typename sint::bit_type> Procb;
  SubProcessor<sgf2n> Proc2;
  SubProcessor<sint>  Procp;

  typename sgf2n::PrivateOutput privateOutput2;
  typename sint::PrivateOutput privateOutputp;

  unsigned int PC;
  TempVars<sint, sgf2n> temp;

  PRNG shared_prng;

  ExternalClients external_clients;
  Binary_File_IO binary_file_io;
  
  // avoid re-computation of expensive division
  map<int, typename sint::clear> inverses2m;

  SwitchableOutput out;

  void reset(const Program& program,int arg); // Reset the state of the processor
  string get_filename(const char* basename, bool use_number);

  Processor(int thread_num,Player& P,
          typename sgf2n::MAC_Check& MC2,typename sint::MAC_Check& MCp,
          Machine<sint, sgf2n>& machine,
          const Program& program);
  ~Processor();

  int get_thread_num()
    {
      return thread_num;
    }
    const typename sgf2n::clear& read_C2(int i) const
      { return Proc2.C[i]; }
    const sgf2n& read_S2(int i) const
      { return Proc2.S[i]; }
    typename sgf2n::clear& get_C2_ref(int i)
      { return Proc2.C[i]; }
    sgf2n& get_S2_ref(int i)
      { return Proc2.S[i]; }
    void write_C2(int i,const typename sgf2n::clear& x)
      { Proc2.C[i]=x; }
    void write_S2(int i,const sgf2n& x)
      { Proc2.S[i]=x; }
  
    const typename sint::clear& read_Cp(int i) const
      { return Procp.C[i]; }
    const sint & read_Sp(int i) const
      { return Procp.S[i]; }
    typename sint::clear& get_Cp_ref(int i)
      { return Procp.C[i]; }
    sint & get_Sp_ref(int i)
      { return Procp.S[i]; }
    void write_Cp(int i,const typename sint::clear& x)
      { Procp.C[i]=x; }
    void write_Sp(int i,const sint & x)
      { Procp.S[i]=x; }

  void dabit(const Instruction& instruction);
  void edabit(const Instruction& instruction, bool strict = false);

  void convcbitvec(const Instruction& instruction);
  void convcintvec(const Instruction& instruction);
  void convcbit2s(const Instruction& instruction);
  void split(const Instruction& instruction);

  // Access to external client sockets for reading clear/shared data
  void read_socket_ints(int client_id, const vector<int>& registers);
  
  void write_socket(const RegType reg_type, const SecrecyType secrecy_type, const bool send_macs,
                             int socket_id, int message_type, const vector<int>& registers);

  void read_socket_vector(int client_id, const vector<int>& registers);
  void read_socket_private(int client_id, const vector<int>& registers, bool send_macs);

  // Read and write secret numeric data to file (name hardcoded at present)
  void read_shares_from_file(int start_file_pos, int end_file_pos_register, const vector<int>& data_registers);
  void write_shares_to_file(const vector<int>& data_registers);
  
  // Print the processor state
  template<class T, class U>
  friend ostream& operator<<(ostream& s,const Processor<T, U>& P);

  private:

  template<class T> friend class SPDZ;
  template<class T> friend class SubProcessor;
};

#endif

