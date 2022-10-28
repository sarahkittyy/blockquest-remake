export default function isValidLevel(code: string): boolean {
	const levelRegex = /^(?:[0-9]{3}|\/){1024}/g;
	if (code.match(/^[/0-9]{1023,3073}$/) == null) return false;
	if (code.match(levelRegex) == null) return false;
	return true;
}
