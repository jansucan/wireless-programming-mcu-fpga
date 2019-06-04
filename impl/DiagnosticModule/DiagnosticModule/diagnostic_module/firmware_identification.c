/* Author: Jan Sucan */


/**
 * @brief Vytvori z mena a Git shorthash identifikacny retazec aplikacie.
 *
 * @param dest           Buffer pre ulozenie vysledneho ID retazca.
 * @param app_name       Meno aplikacie.
 * @param git_shorthash  Git shorthash.
 *
 * @return Dlzka retazca bez ukoncovacieho nuloveho bajtu.
 */
int
firmware_identification_string(char * dest, const char * app_name, const char * git_shorthash)
{
	// Povodna hodnota pointeru na buffer pre vypocet dlzky skonstruovaneho ID retazca
	char * dest_orig = dest;
	// Najprv bude nazov aplikacie
	while (*app_name != '\0') {
		*(dest++) = *(app_name++);
	}
	// Medzera
	*(dest++) = ' ';
	// Nakoniec Git shorthash
	while (*git_shorthash != '\0') {
		*(dest++) = *(git_shorthash++);
	}
	return dest - dest_orig;
}