/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "endpoint_list.hpp"
#include "logger.hpp"
#include "logging_context.hpp"
#include "scoped_thread.hpp"
#include "streambuf_backend.hpp"
#include "tcp_connection.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <exception>
#include <iostream>
#include <tuple>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace xes;

char const lorem[] = 
R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas
in velit enim. Nulla sollicitudin, metus in feugiat pretium, odio ante
molestie urna, vitae dictum sem quam faucibus lacus. Curabitur gravida
bibendum convallis. Duis vulputate posuere sodales. Nulla faucibus
elementum ligula, sit amet semper augue volutpat ac. Donec in metus
euismod, semper velit at, volutpat nisi. Nam et nibh viverra turpis
vulputate malesuada sed non risus. Vestibulum et ornare purus. Ut
vulputate metus ut lacus aliquet, et gravida lacus lacinia. Vivamus
vel neque id dolor fringilla fermentum. Fusce cursus justo et erat
sagittis, in porttitor libero eleifend.

Fusce aliquet ligula et lectus fermentum consequat sed auctor
nunc. Aliquam mollis malesuada eros, vel aliquam sem. In nec est
porttitor, iaculis leo id, mattis turpis. Mauris lobortis viverra
lectus, et blandit libero commodo vitae. Duis vitae iaculis
urna. Donec pretium ante eu convallis accumsan. Sed a luctus
ipsum. Duis vitae sem ac lorem tincidunt fermentum eget quis
risus. Proin sodales ex a elit venenatis, id ullamcorper est
eleifend. Nam risus erat, elementum vel eros eget, interdum ultrices
erat. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris
tincidunt augue vel augue iaculis malesuada. Proin luctus sit amet
augue in feugiat. Nam maximus leo vitae vulputate lacinia.

Integer mi elit, dignissim eu egestas quis, commodo ac enim. Phasellus
et tortor in lectus interdum posuere a nec arcu. Duis varius gravida
lacinia. Pellentesque tortor orci, vehicula eu enim a, bibendum
blandit eros. Phasellus at efficitur nunc. Phasellus sollicitudin
justo enim, non eleifend ante facilisis at. Suspendisse
potenti. Praesent fringilla quam eget ultricies feugiat. Nam in
convallis tellus. Curabitur nec nibh a sapien pharetra molestie.

Ut hendrerit mattis massa, at posuere metus sagittis quis. Phasellus
sodales leo et quam pellentesque efficitur. Nullam a lectus a velit
condimentum dignissim sed nec orci. Maecenas non commodo risus. Mauris
lorem orci, accumsan quis eleifend nec, iaculis non elit. Sed sed
viverra nisl. Mauris mollis ultricies malesuada. Pellentesque
efficitur quam ante, vel commodo mi dignissim sit amet. Integer
suscipit, nisl in faucibus interdum, eros ex ultricies est, non cursus
sapien libero id mauris. Cras maximus lorem vel lorem vulputate,
semper posuere dolor convallis. Fusce sed felis egestas, pulvinar nibh
sit amet, tincidunt tortor. Donec luctus elit facilisis efficitur
luctus. Quisque suscipit at nulla eget sodales. Vivamus quis sagittis
elit.

Proin rutrum eleifend sagittis. Curabitur tincidunt sodales sapien sed
tempus. Donec eget mi vitae est porttitor dignissim quis nec
leo. Fusce elementum lacus ac massa auctor tincidunt sed eu
lacus. Aenean ultrices velit velit. Cras nec iaculis quam. Quisque ac
mauris quis sem maximus egestas nec volutpat tortor. Vestibulum
elementum nisi leo, vitae semper dolor posuere vel. Praesent justo
libero, pretium sed elit eu, cursus viverra nunc. Sed fermentum nisi
vitae mi laoreet dictum. Praesent odio ligula, tincidunt sit amet
vulputate non, ornare ut mauris.

Sed iaculis pretium dignissim. Vivamus eget porttitor lectus. Integer
nisl lectus, elementum eu feugiat ac, luctus eget purus. Curabitur a
ipsum ac mauris lobortis blandit. Sed non varius nulla, ac auctor
mauris. Sed ornare, justo quis feugiat faucibus, nunc arcu accumsan
mauris, vel ornare ex massa sit amet ante. Mauris condimentum mollis
ante, eget viverra dui eleifend sit amet. Aenean vehicula mauris ac
orci egestas, vel malesuada velit mollis.

Donec venenatis luctus neque id auctor. Nullam sed mattis erat, id
semper enim. Nunc gravida justo diam, nec pharetra lorem lacinia
ac. Nulla sit amet rutrum diam. In viverra augue sit amet nisl euismod
vehicula. Maecenas posuere, magna id imperdiet mollis, magna massa
bibendum tortor, sed tristique nisl nunc dapibus metus. Quisque
dignissim urna sed elit lobortis facilisis. Aenean pulvinar molestie
erat, sed tristique purus tempus nec. Morbi id auctor purus. Aliquam
sed tortor est. Ut lacinia lacus in quam blandit, eget dignissim elit
blandit. Sed at luctus ipsum, et iaculis justo. Proin dapibus lacinia
velit id pellentesque. Vestibulum lacinia purus nisl, sit amet tempus
est consectetur vel. Duis hendrerit elit quis nisi blandit ornare.

Sed cursus congue purus, non commodo purus auctor vitae. Fusce sodales
vehicula turpis, sed semper risus hendrerit vel. Nam eget dui eu leo
egestas dictum eget ut justo. Sed ac dui lorem. Sed interdum
scelerisque eleifend. Nullam hendrerit bibendum dui. Vivamus ac est ac
mi facilisis lobortis. Ut condimentum sed turpis sed venenatis.

Pellentesque interdum elit at interdum varius. Morbi quis erat eu
magna accumsan tristique. Duis vel sagittis tortor. Nulla bibendum,
neque non laoreet auctor, erat turpis consequat erat, sed tempus eros
augue quis odio. Maecenas eu pellentesque neque. Etiam accumsan sed
magna non mattis. Nam porttitor sollicitudin ligula, nec efficitur
ipsum accumsan ac. Fusce vel porta risus, ac ultrices leo. Morbi porta
diam id rhoncus imperdiet. Quisque vel erat in nibh convallis
mattis. Sed ac risus rutrum, tincidunt felis sed, convallis augue.

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla
facilisi. Etiam tempor dui ipsum, at rutrum nunc commodo sed. Donec ac
tincidunt dui, in porttitor felis. Curabitur egestas iaculis
rutrum. Vestibulum lacus metus, feugiat a molestie eget, mollis a
orci. Duis nisl nisi, consectetur quis imperdiet ut, tempus sit amet
nunc. Aliquam blandit accumsan suscipit. Praesent fermentum pretium
mollis. Morbi consequat ornare augue, sit amet tincidunt dolor
porttitor quis. Sed congue vulputate augue et bibendum. Vestibulum
lobortis quis augue quis vulputate. Morbi ut rutrum tortor. Proin
ultrices sem sit amet maximus congue. Maecenas bibendum mollis ipsum,
a molestie magna. Proin nisi lectus, luctus sed mattis non, congue vel
erat.

In at efficitur eros, vel maximus odio. Aliquam malesuada ut justo a
hendrerit. Pellentesque id lectus id ligula semper mattis ut eu
diam. Nullam ultrices, purus et elementum finibus, augue nunc congue
neque, ullamcorper lobortis ante tortor in turpis. Ut placerat ornare
dolor, vel condimentum eros pharetra in. Praesent at aliquam sapien,
sollicitudin porttitor velit. Sed nec augue eu quam eleifend
consectetur. Donec non felis eu justo vulputate
porttitor. Pellentesque eget consequat nulla. Vivamus ornare libero
erat, in congue sapien egestas id. Donec neque lectus, molestie quis
diam quis, imperdiet scelerisque ante. Etiam posuere eleifend augue in
mattis. Curabitur scelerisque iaculis lectus, vitae sollicitudin justo
dictum ac.

Phasellus a quam eget mi sodales vehicula. Etiam a scelerisque
sapien. Nulla ac leo nunc. Vivamus et lorem eget tortor finibus
rhoncus non sed urna. Mauris in purus erat. Mauris vitae elit sit amet
lacus egestas rhoncus sed molestie sem. Nam vulputate eros ante, a
mollis tortor pulvinar ac. Nunc volutpat sapien eros, id volutpat dui
molestie ut. Pellentesque auctor ullamcorper orci, vel varius
est. Fusce rhoncus leo eu sodales aliquam. Morbi varius ante et augue
ullamcorper tincidunt. Vivamus scelerisque nisl nunc, sed auctor dolor
consequat et. Quisque lacinia arcu et dolor varius, ut posuere sapien
eleifend. Sed pellentesque viverra sagittis. Pellentesque ipsum
sapien, finibus ac accumsan quis, efficitur vel dolor. Praesent non mi
odio.

Phasellus vel elit in ligula congue congue. Nullam eget mattis
nibh. Fusce non ex urna. Praesent sodales nisi nec metus dapibus, sed
euismod enim hendrerit. Suspendisse congue interdum felis sed
ornare. In non erat lobortis, ornare nulla eu, convallis lectus. Cras
fringilla urna convallis nibh laoreet ornare. Nunc id urna eu tellus
dapibus tempus. Cras commodo, ipsum in faucibus finibus, odio eros
finibus metus, eget scelerisque sem nulla eget ante. Donec luctus
aliquam dolor vel aliquet. Mauris luctus lobortis tortor, in congue
felis facilisis eget.

Aenean congue ligula a magna malesuada, sit amet accumsan neque
aliquam. Quisque in elit eget enim volutpat ornare. Donec at volutpat
sapien. Maecenas consequat varius vestibulum. Nullam eu massa id
tortor suscipit mollis. In faucibus tellus in sem blandit
commodo. Aenean luctus mauris ac risus volutpat sodales.

Aliquam id placerat elit. Donec gravida vulputate turpis. Integer urna
sem, viverra ut orci vitae, consectetur ullamcorper eros. Ut venenatis
pulvinar venenatis. Suspendisse posuere, nisl ac laoreet blandit,
dolor est pharetra diam, eu ultrices neque arcu sit amet
nisi. Praesent interdum at ipsum ac egestas. In non sem ex.

Cras neque diam, volutpat sodales euismod id, dictum non
nunc. Curabitur quam libero, dictum id mauris vitae, tincidunt
consectetur sem. Quisque diam urna, ornare ac posuere a, mattis
tincidunt diam. Nam mollis vulputate quam, sed faucibus velit tempor
ut. Nunc mollis tempus quam, et fringilla ligula bibendum
et. Vestibulum pulvinar hendrerit justo, a iaculis dui commodo
in. Duis convallis eu ex pretium elementum. Donec at ligula sem. Etiam
tincidunt sapien maximus erat iaculis, eget tincidunt orci
euismod. Vivamus ac pretium lacus. Nam eleifend turpis elit, eu
eleifend nulla eleifend a. Mauris rutrum venenatis suscipit. Aliquam
eleifend magna eleifend nisi fermentum, at dictum velit auctor.

Curabitur auctor ex maximus dictum interdum. Duis eget lectus sit amet
ipsum malesuada efficitur. Vestibulum ullamcorper at diam at
lobortis. Nullam in pulvinar sapien. Quisque laoreet ut nisi a
dapibus. Proin non vestibulum ligula. Fusce sagittis lobortis tellus,
ac volutpat sem. Integer gravida sem in purus convallis, id imperdiet
dui molestie. Curabitur eu imperdiet lectus. Pellentesque vel ultrices
orci. Cras euismod finibus mattis. Lorem ipsum dolor sit amet,
consectetur adipiscing elit.

Donec a blandit tellus, vel mollis metus. Proin scelerisque nulla nec
arcu congue imperdiet. In id molestie sapien. In hac habitasse platea
dictumst. Mauris aliquet vehicula neque non placerat. Curabitur in
lacus vulputate, mollis dolor a, convallis ipsum. Donec id nisl
ex. Phasellus aliquam nisl eget auctor faucibus. Cras in feugiat
ipsum. Vestibulum vestibulum id diam non tristique.

Fusce bibendum sagittis tortor, a mollis ipsum volutpat sit amet. Nam
imperdiet dui tortor, sed maximus eros mattis ac. Nunc varius est
justo, rhoncus imperdiet nisi sollicitudin in. Suspendisse a tristique
tortor. Sed fermentum mattis nibh, eu sollicitudin tortor tincidunt
id. Nunc eget nulla ac nisl viverra congue. Quisque lacinia, felis vel
laoreet dictum, sapien nunc faucibus mi, sit amet tincidunt augue
risus eu magna. Curabitur eget dui cursus, pretium mauris eu,
ultricies diam. Nunc sagittis elit lorem, eu condimentum lectus
faucibus ac. Pellentesque neque risus, fermentum nec commodo id,
dapibus at sapien.

Suspendisse sodales libero sed nisi tempus bibendum. Etiam ultricies
vel dolor at facilisis. Vestibulum consequat enim in consequat
ultrices. Vestibulum bibendum elementum nisl, vel placerat magna
vestibulum vitae. Etiam a rhoncus urna, id maximus mi. Ut molestie
ligula eros. Nulla consequat congue ligula et gravida. Aliquam ut sem
a enim viverra imperdiet quis eu mauris. Sed ornare euismod lectus id
vulputate. Cras turpis augue, malesuada vel ex id, vestibulum
dignissim nisi. Curabitur lorem ex, facilisis quis bibendum sed,
consectetur ac urna. Etiam porta hendrerit ex sit amet
hendrerit. Mauris ac diam facilisis ex egestas ultricies.

Nam elit nunc, vehicula vitae finibus ut, vestibulum ut purus. Donec
sed dolor vitae eros varius porta. Pellentesque habitant morbi
tristique senectus et netus et malesuada fames ac turpis egestas. Sed
congue leo id leo lacinia vehicula. Quisque euismod, dui sit amet
consectetur fermentum, neque turpis semper quam, et pellentesque
ligula justo non eros. Nullam ut est sit amet sapien lacinia
bibendum. Pellentesque habitant morbi tristique senectus et netus et
malesuada fames ac turpis egestas. Donec pellentesque lorem ac dolor
sodales auctor. Ut hendrerit tristique dolor, fermentum venenatis diam
scelerisque quis. Vivamus et sem iaculis, rhoncus libero id, mollis
justo. Nunc sed nisi lacinia, molestie leo eget, sagittis
lorem. Phasellus suscipit neque dolor, non efficitur augue fermentum
ut. Proin viverra libero sit amet nisi varius sodales. Donec posuere
risus vitae tempus venenatis. Nam eu ante congue, eleifend ligula id,
bibendum tellus. Praesent eget sem volutpat, iaculis neque sed,
ullamcorper velit.

Vivamus quis arcu vulputate, luctus neque nec, blandit neque. Donec
hendrerit tortor nec purus porta, in efficitur felis tincidunt. Fusce
consequat nisi et augue finibus, non porttitor metus commodo. Vivamus
ut pharetra urna. Vivamus imperdiet magna in ipsum sodales
viverra. Vivamus sapien mauris, semper sit amet diam non, pharetra
fringilla enim. Integer sem diam, dictum a nisl imperdiet, imperdiet
venenatis enim. Aliquam erat volutpat.

Curabitur eu erat vel tortor vestibulum faucibus. Class aptent taciti
sociosqu ad litora torquent per conubia nostra, per inceptos
himenaeos. Cras elementum ex quis tortor venenatis, ac vehicula lorem
hendrerit. Quisque euismod leo in sapien iaculis tincidunt. Praesent
diam leo, efficitur vel urna et, ornare tincidunt tortor. Donec
feugiat elit nec nibh scelerisque, vel mattis magna
pellentesque. Nulla nec tortor tincidunt, porta mi sit amet, tincidunt
purus. Aliquam vitae feugiat tellus. Sed sed euismod diam. Duis
tincidunt lacus at ipsum sodales efficitur.

Sed rutrum sagittis purus a pulvinar. Phasellus varius ligula
porttitor, mollis magna nec, lacinia sapien. Cras faucibus faucibus
leo id faucibus. Proin nisl nisl, feugiat vitae libero vitae,
tristique placerat ipsum. Nam congue gravida gravida. In vel velit
convallis orci mollis fringilla. Etiam sollicitudin ultricies
lobortis. Curabitur id ligula iaculis, sodales dolor in, tincidunt
erat. Donec varius mauris non nisl tincidunt, nec pulvinar dolor
faucibus. Morbi id mi consequat, consequat odio bibendum, rutrum
lacus. Integer sodales luctus justo non tempus.

Aenean pharetra, arcu eu fringilla suscipit, lectus ipsum ornare arcu,
a vestibulum dolor odio id libero. Sed at porta justo, vel venenatis
tortor. Sed et velit pellentesque, ullamcorper turpis sit amet, mattis
magna. Sed gravida fringilla arcu, at aliquam nibh fermentum eget. In
sagittis malesuada tristique. Nam accumsan, purus non convallis
bibendum, tortor erat pulvinar nulla, et varius ante arcu sed
ante. Donec diam eros, pharetra et rutrum eu, tristique scelerisque
eros. Nulla porta in magna ac facilisis. Praesent nisi ex, accumsan et
scelerisque sit amet, suscipit eu justo. Curabitur sit amet
condimentum ipsum. Etiam consequat est in diam efficitur, ut venenatis
tellus porttitor. Nullam congue ante non varius cursus. Morbi nec enim
sed leo ornare lacinia. Nullam ac fermentum risus, ut dictum
purus. Nullam consequat rutrum venenatis. Nullam ut nisl mollis,
tempus elit vel, eleifend sem.
)"; // lorem

std::string make_lorems(unsigned int n)
{
  std::string result;
  while(n--)
  {
    result += lorem;
  }
  return result;
}

struct producer_t
{
  producer_t(logging_context_t const& context,
             tcp_connection_t& out,
             char const* first, char const* last,
             int bufsize)
  : context_(context)
  , out_(out)
  , first_(first)
  , last_((assert(last >= first), last))
  , bufsize_((assert(bufsize > 0), bufsize))
  , done_(false)
  { }

  producer_t(producer_t const&) = delete;
  producer_t& operator=(producer_t const&) = delete;

  bool done() const
  {
    return done_;
  }

  bool progress()
  {
    if(done_)
    {
      return false;
    }

    if(first_ == last_)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "producer " << out_ << ": closing write end";
      }
      out_.close_write_end();
      done_ = true;
      return true;
    }

    char const* limit = last_;
    if(limit - first_ > bufsize_)
    {
      limit = first_ + bufsize_;
    }
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "producer " << out_ <<
        ": trying to send " << limit - first_ << " bytes";
    }

    char const* next = out_.write_some(first_, limit);
    if(next == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "producer " << out_ << ": can't send yet";
      }
      return false;
    }

    assert(next > first_);
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "producer " << out_ <<
        ": sent " << next - first_ << " bytes";
    }

    first_ = next;
      
    return true;
  }

private :
  logging_context_t const& context_;
  tcp_connection_t& out_;
  char const* first_;
  char const* last_;
  int bufsize_;
  bool done_;
};

struct echoer_t
{
  echoer_t(logging_context_t const& context,
           tcp_connection_t& in, tcp_connection_t& out,
           char const* first, char const* last,
           int bufsize)
  : context_(context)
  , in_(in)
  , out_(out)
  , eof_seen_(false)
  , done_(false)
  , first_(first)
  , expected_(first)
  , last_((assert(last >= first), last))
  , bufsize_((assert(bufsize > 0), bufsize))
  , buf_(new char[bufsize_])
  { }

  echoer_t(echoer_t const&) = delete;
  echoer_t& operator=(echoer_t const&) = delete;

  bool done() const
  {
    return done_;
  }

  bool progress()
  {
    return try_close_write_end()
      || try_read_eof()
      || try_write_data()
      || try_read_data();
  }
   
  ~echoer_t()
  {
    delete[] buf_;
  }

private :
  bool try_read_data()
  {
    if(expected_ == last_)
    {
      return false;
    }

    char* limit = buf_ + bufsize_;
    if(limit - buf_ > last_ - expected_)
    {
      limit = buf_ + (last_ - expected_);
    }

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "echoer " << in_ <<
        ": trying to receive " << limit - buf_ << " bytes";
    }

    char* next = in_.read_some(buf_, limit);
    if(next == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "echoer " << in_ << ": nothing to receive yet";
      }
      return false;
    }

    assert(next > buf_);
    assert(std::equal(buf_, next, expected_));
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "echoer " << in_ <<
        ": received " << next - buf_ << " bytes";
    }

    expected_ += next - buf_;
    return true;
  }

  bool try_read_eof()
  {
    if(expected_ != last_ || eof_seen_)
    {
      return false;
    }
    
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "echoer " << in_ << ": trying to receive EOF";
    }

    char dummy[1];
    char* next = in_.read_some(dummy, dummy + 1);
    if(next == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "echoer " << in_ << ": nothing to receive yet";
      }
      return false;
    }

    assert(next == dummy);
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "echoer " << in_ << ": received EOF";
    }

    eof_seen_ = true;
    return true;
  }

  bool try_write_data()
  {
    if(first_ == expected_)
    {
      return false;
    }

    char const* limit = expected_;
    if(expected_ - first_ > bufsize_)
    {
      limit = first_ + bufsize_;
    }

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "echoer " << out_ <<
        ": trying to send " << limit - first_ << " bytes";
    }

    char const* next = out_.write_some(first_, limit);
    if(next == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "echoer " << out_ << ": can't send yet";
      }
      return false;
    }

    assert(next > first_);
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "echoer " << out_ <<
        ": sent " << next - first_ << " bytes";
    }

    first_ = next;
    return true;
  }

  bool try_close_write_end()
  {
    if(first_ != last_ || !eof_seen_ || done_)
    {
      return false;
    }

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "echoer " << out_ << ": closing write end";
    }

    out_.close_write_end();
    done_ = true;
    return true;
  }

private :
  logging_context_t const& context_;
  tcp_connection_t& in_;
  tcp_connection_t& out_;
  bool eof_seen_;
  bool done_;
  char const* first_;
  char const* expected_;
  char const* last_;
  int bufsize_;
  char* const buf_;
};
  
struct consumer_t
{
  consumer_t(logging_context_t const& context,
             tcp_connection_t& in,
             char const* first, char const* last,
             int bufsize)
  : context_(context)
  , in_(in)
  , done_(false)
  , first_(first)
  , last_((assert(last >= first), last))
  , bufsize_((assert(bufsize > 0), bufsize))
  , buf_(new char[bufsize_])
  { }

  consumer_t(consumer_t const&) = delete;
  consumer_t& operator=(consumer_t const&) = delete;

  bool done() const
  {
    return done_;
  }

  bool progress()
  {
    if(done_)
    {
      return false;
    }

    if(first_ == last_)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "consumer " << in_ << ": trying to receive EOF";
      }

      char dummy[1];
      char* next = in_.read_some(dummy, dummy + 1);
      if(next == nullptr)
      {
        if(auto msg = context_.message_at(loglevel_t::info))
        {
          *msg << "consumer " << in_ << ": nothing to receive yet";
        }
        return false;
      }

      assert(next == dummy);
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "consumer " << in_ << ": received EOF";
      }

      done_ = true;
      return true;
    }

    char* limit = buf_ + bufsize_;
    if(last_ - first_ < bufsize_)
    {
      limit = buf_ + (last_ - first_);
    }
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "consumer " << in_ <<
        ": trying to receive " << limit - buf_ << " bytes";
    }

    char* next = in_.read_some(buf_, limit);
    if(next == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "consumer " << in_ << ": nothing to receive yet";
      }
      return false;
    }

    assert(next > buf_);
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "consumer " << in_ <<
        ": received " << next - buf_ << " bytes";
    }

    assert(std::equal(buf_, next, first_));
    first_ += next - buf_;
      
    return true;
  }

  ~consumer_t()
  {
    delete[] buf_;
  }

private :
  logging_context_t const& context_;
  tcp_connection_t& in_;
  bool done_;
  char const* first_;
  char const* last_;
  int bufsize_;
  char* buf_;
};
  
int const bufsize = 256 * 1024;
unsigned int const n_lorems = 256;

void blocking_transfer(logging_context_t const& context,
                       endpoint_t const& interface)
{
  std::string const payload = make_lorems(n_lorems);
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  std::unique_ptr<tcp_connection_t> producer_out;
  std::unique_ptr<tcp_connection_t> echoer_in;
  std::tie(producer_out, echoer_in) = make_connected_pair(interface);
  
  std::unique_ptr<tcp_connection_t> echoer_out;
  std::unique_ptr<tcp_connection_t> consumer_in;
  std::tie(echoer_out, consumer_in) = make_connected_pair(interface);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "blocking_transfer():" <<
      " producer out: " << *producer_out <<
      " echoer in: " << *echoer_in <<
      " echoer out: " << *echoer_out <<
      " consumer in: " << *consumer_in <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size();
  }

  producer_t producer(context, *producer_out, first, last, bufsize);
  auto produce = [&]
  {
    while(!producer.done())
    {
      bool progressed = producer.progress();
      assert(progressed);
    }
  };

  echoer_t echoer(context, *echoer_in, *echoer_out, first, last, bufsize);
  auto echo = [&]
  {
    while(!echoer.done())
    {
      bool progressed = echoer.progress();
      assert(progressed);
    }
  };

  consumer_t consumer(context, *consumer_in, first, last, bufsize);
  auto consume = [&]
  {
    while(!consumer.done())
    {
      bool progressed = consumer.progress();
      assert(progressed);
    }
  };

  {
    scoped_thread_t producer_thread(produce);
    scoped_thread_t echoer_thread(echo);
    consume();
  }
}

void blocking_transfer(logging_context_t const& context)
{
  endpoint_list_t interfaces(local_interfaces, any_port);
  for(auto const& interface : interfaces)
  {
    blocking_transfer(context, interface);
  }
}
      
void nonblocking_transfer(logging_context_t const& context,
                          endpoint_t const& interface,
                          bool agile)
{
  std::string const payload = make_lorems(n_lorems);
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  std::unique_ptr<tcp_connection_t> producer_out;
  std::unique_ptr<tcp_connection_t> echoer_in;
  std::tie(producer_out, echoer_in) = make_connected_pair(interface);

  producer_out->set_nonblocking();
  echoer_in->set_nonblocking();
  
  std::unique_ptr<tcp_connection_t> echoer_out;
  std::unique_ptr<tcp_connection_t> consumer_in;
  std::tie(echoer_out, consumer_in) = make_connected_pair(interface);

  echoer_out->set_nonblocking();
  consumer_in->set_nonblocking();
  
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "nonblocking_transfer():" <<
      " producer out: " << *producer_out <<
      " echoer in: " << *echoer_in <<
      " echoer out: " << *echoer_out <<
      " consumer in: " << *consumer_in <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size() <<
      " agile: " << (agile ? "yes" : "no");
  }

  producer_t producer(context, *producer_out, first, last, bufsize);
  echoer_t echoer(context, *echoer_in, *echoer_out, first, last, bufsize);
  consumer_t consumer(context, *consumer_in, first, last, bufsize);

  while(!consumer.done())
  {
    while(producer.progress() && agile)
      ;
    while(echoer.progress() && agile)
      ;
    while(consumer.progress() && agile)
      ;
  }
}

void nonblocking_transfer(logging_context_t const& context, bool agile)
{
  endpoint_list_t interfaces(local_interfaces, any_port);
  for(auto const& interface : interfaces)
  {
    nonblocking_transfer(context, interface, agile);
  }
}
      
void blocking_client_server(logging_context_t const& context,
                            endpoint_t const& interface)
{
  std::string const payload = make_lorems(n_lorems);
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  std::unique_ptr<tcp_connection_t> client_side;
  std::unique_ptr<tcp_connection_t> server_side;
  std::tie(client_side, server_side) = make_connected_pair(interface);
  
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "blocking_client_server():" <<
      " client side: " << *client_side <<
      " server side: " << *server_side <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size();
  }

  producer_t producer(context, *client_side, first, last, bufsize);
  auto produce = [&]
  {
    while(!producer.done())
    {
      bool progressed = producer.progress();
      assert(progressed);
    }
  };

  echoer_t echoer(context, *server_side, *server_side, first, last, bufsize);
  auto echo = [&]
  {
    while(!echoer.done())
    {
      bool progressed = echoer.progress();
      assert(progressed);
    }
  };
    
  consumer_t consumer(context, *client_side, first, last, bufsize);
  auto consume = [&]
  {
    while(!consumer.done())
    {
      bool progressed = consumer.progress();
      assert(progressed);
    }
  };

  {
    scoped_thread_t producer_thread(produce);
    scoped_thread_t echoer_thread(echo);
    consume();
  }
}

void blocking_client_server(logging_context_t const& context)
{
  endpoint_list_t interfaces(local_interfaces, any_port);
  for(auto const& interface : interfaces)
  {
    blocking_client_server(context, interface);
  }
}
      
void nonblocking_client_server(logging_context_t const& context,
                               endpoint_t const& interface,
                               bool agile)
{
  std::string const payload = make_lorems(n_lorems);
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  std::unique_ptr<tcp_connection_t> client_side;
  std::unique_ptr<tcp_connection_t> server_side;
  std::tie(client_side, server_side) = make_connected_pair(interface);

  client_side->set_nonblocking();
  server_side->set_nonblocking();
  
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "nonblocking_client_server():" <<
      " client side: " << *client_side <<
      " server_side: " << *server_side <<
      " bytes to transfer: " << payload.size() <<
      " agile: " << (agile ? "yes" : "no");
  }

  producer_t producer(context, *client_side, first, last, bufsize);
  echoer_t echoer(context, *server_side, *server_side, first, last, bufsize);
  consumer_t consumer(context, *client_side, first, last, bufsize);

  while(!consumer.done())
  {
    while(producer.progress() && agile)
      ;
    while(echoer.progress() && agile)
      ;
    while(consumer.progress() && agile)
      ;
  }
}

void nonblocking_client_server(logging_context_t const& context, bool agile)
{
  endpoint_list_t interfaces(local_interfaces, any_port);
  for(auto const& interface : interfaces)
  {
    nonblocking_client_server(context, interface, agile);
  }
}
      
void broken_pipe(logging_context_t const& context,
                 endpoint_t const& interface)
{
  std::unique_ptr<tcp_connection_t> producer_out;
  std::unique_ptr<tcp_connection_t> consumer_in;
  std::tie(producer_out, consumer_in) = make_connected_pair(interface);
  
  std::string const payload = make_lorems(n_lorems);
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "broken pipe():" <<
      " producer out: " << *producer_out <<
      " consumer_in (closing): " << *consumer_in <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size();
  }

  consumer_in.reset();
  producer_t producer(context, *producer_out, first, last, bufsize);

  bool caught = false;
  try
  {
    while(!producer.done())
    {
      bool progressed = producer.progress();
      assert(progressed);
    }
  }
  catch(system_exception_t const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "broken_pipe(): caught expected exception: " << ex.what();
    }
    caught = true;
  }
  assert(caught);
}

void broken_pipe(logging_context_t const& context)
{
  endpoint_list_t interfaces(local_interfaces, any_port);
  for(auto const& interface : interfaces)
  {
    broken_pipe(context, interface);
  }
}

int throwing_main(int argc, char const* const argv[])
{
  logger_t logger(argv[0]);
  logger.set_backend(std::make_unique<xes::streambuf_backend_t>(std::cerr));
  logging_context_t context(
    logger, argc == 1 ? loglevel_t::error : loglevel_t::info);

  blocking_transfer(context);
  nonblocking_transfer(context, false);
  nonblocking_transfer(context, true);

  blocking_client_server(context);
  nonblocking_client_server(context, false);
  nonblocking_client_server(context, true);

  broken_pipe(context);

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  int r = 1;

  try
  {
    r = throwing_main(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return r;
}
