/*
 * Copyright (C) 2021-2025 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See version
 * 2.1 of the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cuti/tcp_connection.hpp>

#include <cuti/circular_buffer.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/error_status.hpp>
#include <cuti/file_backend.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/selector.hpp>
#include <cuti/selector_factory.hpp>
#include <cuti/socket_layer.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/streambuf_backend.hpp>

#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

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

struct logged_tcp_connection_t
{
  logged_tcp_connection_t(logging_context_t const& context,
                          loglevel_t loglevel,
                          std::string prefix,
                          tcp_connection_t& conn)
  : context_(context)
  , loglevel_(loglevel)
  , prefix_(std::move(prefix))
  , conn_(conn)
  { }

  logged_tcp_connection_t(logged_tcp_connection_t const&) = delete;
  logged_tcp_connection_t& operator=(logged_tcp_connection_t const&) = delete;

  int write(char const* first, char const* last, char const*& next)
  {
    if(auto msg = context_.message_at(loglevel_))
    {
      *msg << prefix_ << '[' << conn_ <<
        "]: trying to send " << last - first << " byte(s)";
    }

    int result = conn_.write(first, last, next);
    if(result != 0)
    {
      if(auto msg = context_.message_at(loglevel_))
      {
        *msg << prefix_ << '[' << conn_ << "]: reported system error: " <<
          error_status_t(result);
      }
    }

    if(auto msg = context_.message_at(loglevel_))
    {
      if(next == nullptr)
      {
        *msg << prefix_ << '[' << conn_ << "]: can't send yet";
      }
      else
      {
        *msg << prefix_ << '[' << conn_ <<
          "]: sent " << next - first << " byte(s)";
      }
    }

    return result;
  }

  int close_write_end()
  {
    if(auto msg = context_.message_at(loglevel_))
    {
      *msg << prefix_ << '[' << conn_ << "]: sending EOF";
    }

    int result = conn_.close_write_end();
    if(result != 0)
    {
      if(auto msg = context_.message_at(loglevel_))
      {
        *msg << prefix_ << '[' << conn_ << "]: reported system error: " <<
          error_status_t(result);
      }
    }

    return result;
  }

  int read(char* first, char const* last, char*& next)
  {
    if(auto msg = context_.message_at(loglevel_))
    {
      *msg << prefix_ << '[' << conn_ <<
        "]: trying to receive " << last - first << " byte(s)";
    }

    int result = conn_.read(first, last, next);
    if(result != 0)
    {
      if(auto msg = context_.message_at(loglevel_))
      {
        *msg << prefix_ << '[' << conn_ << "]: reported system error: " <<
          error_status_t(result);
      }
    }

    if(auto msg = context_.message_at(loglevel_))
    {
      if(next == nullptr)
      {
        *msg << prefix_ << '[' << conn_ << "] : nothing to receive yet";
      }
      else if(next == first)
      {
        *msg << prefix_ << '[' << conn_ << "]: received EOF";
      }
      else
      {
        *msg << prefix_ << '[' << conn_ <<
          "]: received " << next - first << " byte(s)";
      }
    }

    return result;
  }

  template<typename Callback>
  cancellation_ticket_t call_when_writable(scheduler_t& scheduler,
                                           Callback&& callback)
  {
    if(auto msg = context_.message_at(loglevel_))
    {
      *msg << prefix_ << '[' << conn_ << "]: requesting writable callback";
    }

    return conn_.call_when_writable(
      scheduler, std::forward<Callback>(callback));
  }

  template<typename Callback>
  cancellation_ticket_t call_when_readable(scheduler_t& scheduler,
                                           Callback&& callback)
  {
    if(auto msg = context_.message_at(loglevel_))
    {
      *msg << prefix_ << '[' << conn_ << "]: requesting readable callback";
    }

    return conn_.call_when_readable(
      scheduler, std::forward<Callback>(callback));
  }

  ~logged_tcp_connection_t()
  {
    if(auto msg = context_.message_at(loglevel_))
    {
      *msg << prefix_ << '[' << conn_ << "]: connection destructor";
    }
  }

private :
  logging_context_t const& context_;
  loglevel_t loglevel_;
  std::string prefix_;
  tcp_connection_t& conn_;
};

/*
 * Our test producer sends [first,last>, and then a half-close, to out.
 */
struct producer_t
{
  static const unsigned int max_bufsize = std::numeric_limits<int>::max();

  producer_t(logging_context_t const& context,
             tcp_connection_t& out,
             char const* first, char const* last,
             unsigned int bufsize)
  : out_(context, loglevel_t::debug, "producer", out)
  , bufsize_((assert(bufsize > 0),
              assert(bufsize <= max_bufsize),
              static_cast<int>(bufsize)))
  , first_(first)
  , last_(last)
  , eof_sent_(false)
  , writable_ticket_()
  { }

  producer_t(producer_t const&) = delete;
  producer_t& operator=(producer_t const&) = delete;

  bool done() const
  { return !wants_write(); }

  bool progress()
  { return write_step(); }

  // SSTS: static start takes shared
  static void start(std::shared_ptr<producer_t> const& self,
                    scheduler_t& scheduler)
  {
    if(self->writable_ticket_.empty() && self->wants_write())
    {
      self->writable_ticket_ = self->out_.call_when_writable(
        scheduler,
        [self, &scheduler](stack_marker_t&)
	{ self->on_writable(self, scheduler); }
      );
    }
  }

private :
  bool wants_write() const
  { return !eof_sent_; }

  bool write_step()
  {
    if(eof_sent_)
    {
      return false;
    }

    if(first_ == last_)
    {
      out_.close_write_end();
      eof_sent_ = true;
      return true;
    }

    char const* limit = last_;
    if(limit - first_ > bufsize_)
    {
      limit = first_ + bufsize_;
    }

    char const* next;
    out_.write(first_, limit, next);
    if(next == nullptr)
    {
      return false;
    }

    assert(next > first_);
    assert(next <= limit);
    first_  = next;
    return true;
  }

private :
  void on_writable(std::shared_ptr<producer_t> const& self,
                   scheduler_t& scheduler)
  {
    assert(self.get() == this);

    writable_ticket_.clear();
    write_step();
    start(self, scheduler);
  }

private :
  logged_tcp_connection_t out_;
  int const bufsize_;
  char const* first_;
  char const* const last_;
  bool eof_sent_;
  cancellation_ticket_t writable_ticket_;
};

/*
 * Our test filter drains in, copying it to out, followed by a half-close.
 */
struct filter_t
{
  filter_t(logging_context_t const& context,
           tcp_connection_t& in,
           tcp_connection_t& out,
           unsigned int bufsize)
  : in_(context, loglevel_t::debug, "filter", in)
  , out_(context, loglevel_t::debug, "filter", out)
  , buffer_((assert(bufsize > 0), bufsize))
  , eof_seen_(false)
  , eof_sent_(false)
  , readable_ticket_()
  , writable_ticket_()
  { }

  filter_t(filter_t const&) = delete;
  filter_t& operator=(filter_t const&) = delete;

  bool done() const
  { return !wants_read() && !wants_write(); }

  bool progress()
  { return read_step() || write_step(); }

  // SSTS: static start takes shared
  static void start(std::shared_ptr<filter_t> const& self,
                    scheduler_t& scheduler)
  {
    if(self->readable_ticket_.empty() && self->wants_read())
    {
      self->readable_ticket_ = self->in_.call_when_readable(
        scheduler,
        [self, &scheduler](stack_marker_t&)
	{ self->on_readable(self, scheduler); }
      );
    }

    if(self->writable_ticket_.empty() && self->wants_write())
    {
      self->writable_ticket_ = self->out_.call_when_writable(
        scheduler,
        [self, &scheduler](stack_marker_t&)
	{ self->on_writable(self, scheduler); }
      );
    }
  }

private :
  bool wants_read() const
  { return buffer_.has_slack() && !eof_seen_; }

  bool read_step()
  {
    if(!buffer_.has_slack())
    {
      return false;
    }

    if(eof_seen_)
    {
      return false;
    }

    char* next;
    in_.read(buffer_.begin_slack(), buffer_.end_slack(), next);
    if(next == nullptr)
    {
      return false;
    }

    if(next == buffer_.begin_slack())
    {
       eof_seen_ = true;
       return true;
    }

    assert(next > buffer_.begin_slack());
    assert(next <= buffer_.end_slack());
    buffer_.push_back(next);
    return true;
  }

  bool wants_write() const
  { return buffer_.has_data() || (eof_seen_ && !eof_sent_); }

  bool write_step()
  {
    if(buffer_.has_data())
    {
      char const* next;
        out_.write(buffer_.begin_data(), buffer_.end_data(), next);
      if(next == nullptr)
      {
        return false;
      }

      assert(next > buffer_.begin_data());
      assert(next <= buffer_.end_data());
      buffer_.pop_front(next);
      return true;
    }

    if(eof_seen_ && !eof_sent_)
    {
      out_.close_write_end();
      eof_sent_ = true;
      return true;
    }

    return false;
  }

private :
  void on_readable(std::shared_ptr<filter_t> const& self,
                   scheduler_t& scheduler)
  {
    assert(self.get() == this);

    readable_ticket_.clear();
    read_step();
    start(self, scheduler);
  }

  void on_writable(std::shared_ptr<filter_t> const& self,
                   scheduler_t& scheduler)
  {
    assert(self.get() == this);

    writable_ticket_.clear();
    write_step();
    start(self, scheduler);
  }

private :
  logged_tcp_connection_t in_;
  logged_tcp_connection_t out_;
  circular_buffer_t buffer_;
  bool eof_seen_;
  bool eof_sent_;
  cancellation_ticket_t readable_ticket_;
  cancellation_ticket_t writable_ticket_;
};

/*
 * Our test consumer drains in, checking that it matches [first,last>.
 */
struct consumer_t
{
  consumer_t(logging_context_t const& context,
             tcp_connection_t& in,
             char const* first, char const* last,
             unsigned int bufsize)
  : in_(context, loglevel_t::debug, "consumer", in)
  , buffer_((assert(bufsize > 0), bufsize))
  , first_(first)
  , last_(last)
  , eof_seen_(false)
  , readable_ticket_()
  { }

  bool done() const
  { return !wants_read(); }

  bool progress()
  { return read_step(); }

  // SSTS: static start takes shared
  static void start(std::shared_ptr<consumer_t> const& self,
                    scheduler_t& scheduler)
  {
    if(self->readable_ticket_.empty() && self->wants_read())
    {
      self->readable_ticket_ = self->in_.call_when_readable(
        scheduler,
        [self, &scheduler](stack_marker_t&)
        { self->on_readable(self, scheduler); }
      );
    }
  }

private :
  bool wants_read() const
  { return !eof_seen_; }

  bool read_step()
  {
    if(eof_seen_)
    {
      return false;
    }

    assert(buffer_.has_slack());
    char* next;
    in_.read(buffer_.begin_slack(), buffer_.end_slack(), next);
    if(next == nullptr)
    {
      return false;
    }
    assert(next >= buffer_.begin_slack());
    assert(next <= buffer_.end_slack());
    buffer_.push_back(next);

    if(!buffer_.has_data())
    {
      assert(first_ == last_);
      eof_seen_ = true;
      return true;
    }

    assert(buffer_.end_data() - buffer_.begin_data() <= last_ - first_);
    assert(std::equal(buffer_.begin_data(), buffer_.end_data(), first_));

    first_ += buffer_.end_data() - buffer_.begin_data();
    buffer_.pop_front(buffer_.end_data());
    
    return true;
  }

private :
  void on_readable(std::shared_ptr<consumer_t> const& self,
                   scheduler_t& scheduler)
  {
    assert(self.get() == this);

    readable_ticket_.clear();
    read_step();
    start(self, scheduler);
  }

private :
  logged_tcp_connection_t in_;
  circular_buffer_t buffer_;
  char const* first_;
  char const* const last_;
  bool eof_seen_;
  cancellation_ticket_t readable_ticket_;
};

template<typename T>
void run_to_completion(T& function)
{
  while(!function.done())
  {
    bool progressed = function.progress();
    assert(progressed);
  }
}

void run_pipe_in_parallel(producer_t& producer,
                          filter_t& filter,
                          consumer_t& consumer)
{
  scoped_thread_t producer_thread([&] { run_to_completion(producer); });
  scoped_thread_t filter_thread([&] { run_to_completion(filter); });
  run_to_completion(consumer);
}

void run_pipe_serially(producer_t& producer,
                       filter_t& filter,
                       consumer_t& consumer,
                       bool agile)
{
  while(!consumer.done())
  {
    while(producer.progress() && agile)
      ;
    while(filter.progress() && agile)
      ;
    while(consumer.progress() && agile)
      ;
  }
}

unsigned int const bufsize = 256 * 1024;
std::string const payload = make_lorems(256);

void blocking_transfer(logging_context_t const& context,
                       socket_layer_t& sockets,
                       endpoint_t const& interface)
{
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  auto[producer_out, filter_in] = make_connected_pair(sockets, interface);
  auto[filter_out, consumer_in] = make_connected_pair(sockets, interface);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "blocking_transfer():" <<
      " producer out: " << *producer_out <<
      " filter in: " << *filter_in <<
      " filter out: " << *filter_out <<
      " consumer in: " << *consumer_in <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size();
  }

  producer_t producer(context, *producer_out, first, last, bufsize);
  filter_t filter(context, *filter_in, *filter_out, bufsize);
  consumer_t consumer(context, *consumer_in, first, last, bufsize);

  run_pipe_in_parallel(producer, filter, consumer);
}

void blocking_transfer(logging_context_t const& context)
{
  socket_layer_t sockets;

  auto interfaces = local_interfaces(sockets, any_port);
  for(auto const& interface : interfaces)
  {
    blocking_transfer(context, sockets, interface);
  }
}

void nonblocking_transfer(logging_context_t const& context,
                          socket_layer_t& sockets,
                          endpoint_t const& interface,
                          bool agile)
{
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  auto[producer_out, filter_in] = make_connected_pair(sockets, interface);
  auto[filter_out, consumer_in] = make_connected_pair(sockets, interface);

  producer_out->set_nonblocking();
  filter_in->set_nonblocking();
  filter_out->set_nonblocking();
  consumer_in->set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "nonblocking_transfer():" <<
      " producer out: " << *producer_out <<
      " filter in: " << *filter_in <<
      " filter out: " << *filter_out <<
      " consumer in: " << *consumer_in <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size() <<
      " agile: " << (agile ? "yes" : "no");
  }

  producer_t producer(context, *producer_out, first, last, bufsize);
  filter_t filter(context, *filter_in, *filter_out, bufsize);
  consumer_t consumer(context, *consumer_in, first, last, bufsize);

  run_pipe_serially(producer, filter, consumer, agile);
}

void nonblocking_transfer(logging_context_t const& context, bool agile)
{
  socket_layer_t sockets;

  auto interfaces = local_interfaces(sockets, any_port);
  for(auto const& interface : interfaces)
  {
    nonblocking_transfer(context, sockets, interface, agile);
  }
}

void selected_transfer(logging_context_t const& context,
                       socket_layer_t& sockets,
                       selector_factory_t const& factory,
                       endpoint_t const& interface)
{
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  auto[producer_out, filter_in] = make_connected_pair(sockets, interface);
  auto[filter_out, consumer_in] = make_connected_pair(sockets, interface);

  producer_out->set_nonblocking();
  filter_in->set_nonblocking();
  filter_out->set_nonblocking();
  consumer_in->set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "selected_transfer():" <<
      " selector: " << factory <<
      " producer out: " << *producer_out <<
      " filter in: " << *filter_in <<
      " filter out: " << *filter_out <<
      " consumer in: " << *consumer_in <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size();
  }

  default_scheduler_t scheduler(sockets, factory);

  start_event_handler<producer_t>(scheduler,
    context, *producer_out, first, last, bufsize);
  start_event_handler<filter_t>(scheduler,
    context, *filter_in, *filter_out, bufsize);
  auto consumer = start_event_handler<consumer_t>(scheduler,
    context, *consumer_in, first, last, bufsize);

  stack_marker_t base_marker;
  while(auto callback = scheduler.wait())
  {
    callback(base_marker);
  }

  assert(consumer->done());
}

void selected_transfer(logging_context_t const& context)
{
  socket_layer_t sockets;

  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      selected_transfer(context, sockets, factory, interface);
    }
  }
}

void blocking_client_server(logging_context_t const& context,
                            socket_layer_t& sockets,
                            endpoint_t const& interface)
{
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  auto[client_side, server_side] = make_connected_pair(sockets, interface);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "blocking_client_server():" <<
      " client side: " << *client_side <<
      " server side: " << *server_side <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size();
  }

  producer_t producer(context, *client_side, first, last, bufsize);
  filter_t filter(context, *server_side, *server_side, bufsize);
  consumer_t consumer(context, *client_side, first, last, bufsize);

  run_pipe_in_parallel(producer, filter, consumer);
}

void blocking_client_server(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto interfaces = local_interfaces(sockets, any_port);
  for(auto const& interface : interfaces)
  {
    blocking_client_server(context, sockets, interface);
  }
}

void nonblocking_client_server(logging_context_t const& context,
                               socket_layer_t& sockets,
                               endpoint_t const& interface,
                               bool agile)
{
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  auto[client_side, server_side] = make_connected_pair(sockets, interface);

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
  filter_t filter(context, *server_side, *server_side, bufsize);
  consumer_t consumer(context, *client_side, first, last, bufsize);

  run_pipe_serially(producer, filter, consumer, agile);
}

void nonblocking_client_server(logging_context_t const& context, bool agile)
{
  socket_layer_t sockets;
  auto interfaces = local_interfaces(sockets, any_port);
  for(auto const& interface : interfaces)
  {
    nonblocking_client_server(context, sockets, interface, agile);
  }
}

void selected_client_server(logging_context_t const& context,
                            socket_layer_t& sockets,
                            selector_factory_t const& factory,
                            endpoint_t const& interface)
{
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  auto[client_side, server_side] = make_connected_pair(sockets, interface);

  client_side->set_nonblocking();
  server_side->set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "selected_client_server():" <<
      " selector: " << factory <<
      " client side: " << *client_side <<
      " server_side: " << *server_side <<
      " bytes to transfer: " << payload.size();
  }

  default_scheduler_t scheduler(sockets, factory);

  start_event_handler<producer_t>(scheduler,
    context, *client_side, first, last, bufsize);
  start_event_handler<filter_t>(scheduler,
    context, *server_side, *server_side, bufsize);
  auto consumer = start_event_handler<consumer_t>(scheduler,
    context, *client_side, first, last, bufsize);

  stack_marker_t base_marker;
  while(auto callback = scheduler.wait())
  {
    callback(base_marker);
  }

  assert(consumer->done());
}

void selected_client_server(logging_context_t const& context)
{
  socket_layer_t sockets;

  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      selected_client_server(context, sockets, factory, interface);
    }
  }
}

void broken_pipe(logging_context_t const& context,
                 socket_layer_t& sockets,
                 endpoint_t const& interface)
{
  char const* first = payload.data();
  char const* last = payload.data() + payload.size();

  auto[producer_out, consumer_in] = make_connected_pair(sockets, interface);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "broken_pipe():" <<
      " producer out: " << *producer_out <<
      " consumer_in (closing): " << *consumer_in <<
      " buffer size: " << bufsize <<
      " bytes to transfer: " << payload.size();
  }

  consumer_in.reset();
  producer_t producer(context, *producer_out, first, last, bufsize);
  run_to_completion(producer);
}

void broken_pipe(logging_context_t const& context)
{
  socket_layer_t sockets;

  auto interfaces = local_interfaces(sockets, any_port);
  for(auto const& interface : interfaces)
  {
    broken_pipe(context, sockets, interface);
  }
}

void scheduler_switch(logging_context_t const& context,
                      socket_layer_t& sockets,
                      selector_factory_t const& factory,
                      endpoint_t const& interface)
{
  default_scheduler_t sched1(sockets, factory);
  default_scheduler_t sched2(sockets, factory);
  auto [client, server] = make_connected_pair(sockets, interface);

  // put some pressure on the server
  char const msg[] = "Hello server!";
  char const* next;
  client->write(msg, msg + std::strlen(msg), next);
  assert(next != msg);
  assert(next != nullptr);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "scheduler_switch(): selector: " << factory <<
      " client: " << *client << " server: " << *server;
  }

  assert(sched1.wait() == nullptr);
  assert(sched2.wait() == nullptr);

  cancellation_ticket_t writable;
  assert(writable.empty());
  cancellation_ticket_t readable;
  assert(readable.empty());

  writable = client->call_when_writable(sched1, [](stack_marker_t&) { });
  assert(!writable.empty());

  readable = server->call_when_readable(sched1, [](stack_marker_t&) { });
  assert(!readable.empty());

  assert(sched1.wait() != nullptr);
  assert(sched1.wait() != nullptr);
  assert(sched1.wait() == nullptr);
  assert(sched2.wait() == nullptr);

  writable = client->call_when_writable(sched1, [](stack_marker_t&) { });
  assert(!writable.empty());

  readable = server->call_when_readable(sched1, [](stack_marker_t&) { });
  assert(!readable.empty());

  sched1.cancel(writable);
  sched1.cancel(readable);

  writable = client->call_when_writable(sched2, [](stack_marker_t&) { });
  assert(!writable.empty());

  readable = server->call_when_readable(sched2, [](stack_marker_t&) { });
  assert(!readable.empty());

  assert(sched1.wait() == nullptr);
  assert(sched2.wait() != nullptr);
  assert(sched2.wait() != nullptr);
  assert(sched2.wait() == nullptr);

  writable = client->call_when_writable(sched2, [](stack_marker_t&) { });
  assert(!writable.empty());

  readable = server->call_when_readable(sched2, [](stack_marker_t&) { });
  assert(!readable.empty());

  sched2.cancel(writable);
  sched2.cancel(readable);

  writable = client->call_when_writable(sched1, [](stack_marker_t&) { });
  assert(!writable.empty());

  readable = server->call_when_readable(sched1, [](stack_marker_t&) { });
  assert(!readable.empty());

  assert(sched1.wait() != nullptr);
  assert(sched1.wait() != nullptr);
  assert(sched1.wait() == nullptr);
  assert(sched2.wait() == nullptr);
}

void scheduler_switch(logging_context_t const& context)
{
  socket_layer_t sockets;

  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      scheduler_switch(context, sockets, factory, interface);
    }
  }
}

struct options_t
{
  static loglevel_t const default_loglevel = loglevel_t::error;

  options_t()
  : logfile_()
  , logfile_rotation_depth_(file_backend_t::default_rotation_depth)
  , logfile_size_limit_(file_backend_t::no_size_limit)
  , loglevel_(default_loglevel)
  { }

  absolute_path_t logfile_;
  unsigned int logfile_rotation_depth_;
  unsigned int logfile_size_limit_;
  loglevel_t loglevel_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --logfile <name>                  log to a file\n";
  os << "  --logfile-rotation-depth <depth>  set logfile rotation depth " <<
    "(default: " << file_backend_t::default_rotation_depth << ")\n";
  os << "  --logfile-size-limit <limit>      set logfile size limit " <<
    "(default: none)\n";
  os << "  --loglevel <level>                set loglevel " <<
    "(default: " << loglevel_string(options_t::default_loglevel) << ")\n";
  os << std::flush;
}

void read_options(options_t& options, option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match("--logfile", options.logfile_) &&
       !walker.match("--logfile-rotation-depth",
         options.logfile_rotation_depth_) &&
       !walker.match("--logfile-size-limit", options.logfile_size_limit_) &&
       !walker.match("--loglevel", options.loglevel_))
    {
      break;
    }
  }
}

int run_tests(int argc, char const* const* argv)
{
  options_t options;
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  read_options(options, walker);
  if(!walker.done() || !reader.at_end())
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  logger_t logger(std::make_unique<streambuf_backend_t>(std::cerr));
  if(!options.logfile_.empty())
  {
    logger.set_backend(std::make_unique<file_backend_t>(
      options.logfile_,
      options.logfile_size_limit_,
      options.logfile_rotation_depth_));
  }

  logging_context_t context(logger, options.loglevel_);

  blocking_transfer(context);
  nonblocking_transfer(context, false);
  nonblocking_transfer(context, true);
  selected_transfer(context);

  blocking_client_server(context);
  nonblocking_client_server(context, false);
  nonblocking_client_server(context, true);
  selected_client_server(context);

  broken_pipe(context);

  scheduler_switch(context);

  if(auto msg = context.message_at(loglevel_t::info))
  {
      *msg << "tests completed";
  }

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  int r = 1;
  
  try
  {
    r = run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return r;
}
