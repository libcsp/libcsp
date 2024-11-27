#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/csp_id.h>
#include <csp/crypto/csp_hmac.h>

int main(int argc, char * argv[])
{
	csp_packet_t * packet;

	csp_init();

	packet = csp_buffer_get_always();

	csp_id_prepend(packet);

	memcpy(packet->data, "abc", 3);
	packet->length += 3;
	packet->frame_length += 3;

	csp_hmac_append(packet, true);
	csp_hmac_verify(packet, true);

	return 0;
}
